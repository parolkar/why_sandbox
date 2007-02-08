#!/usr/bin/env ruby
require 'sandbox'
$:.unshift File.dirname(__FILE__) + "/../../lib"
%w(open-uri rubygems camping camping/session acts_as_versioned
   json redcloth hpricot cgi pp yaml ostruct).each { |lib| require lib }

Camping.goes :Tepee

module Web
  def self.get(url)
    uri = URI.parse(url)
    data = nil
    case uri
    when URI::HTTP
      data = uri.read
    when URI::Generic
      page = Tepee.get(:Show, url)
      def page.meta; @env end
      def page.base_uri; end
      data = page.body.to_s
      OpenURI::Meta.init(data, page)
    end
    case data.content_type
    when "application/x-javascript", "application/x-json", "application/json"
      obj = JSON.parse(data)
      OpenURI::Meta.init(obj, data)
      obj
    else
      data
    end
  end
end

module Tepee
  include Camping::Session
end

# Jungle module contains non-sandboxed code
# that is called from sandboxed code in the wiki
module Jungle
  # Retrieve a wiki node's raw text
  def self.get_source(wiki_node)
    Tepee.get(:Source,wiki_node).body.to_s
  end
end


module Tepee::Models

  class Page < Base
    PAGE_LINK = /\[\[([^\]|]*)[|]?([^\]]*)\]\]/
    validates_uniqueness_of :title
    # before_save { |r| r.title = r.title.underscore }
    acts_as_versioned
  end

  class CreateTepee < V 1.0
    def self.up
      create_table :tepee_pages, :force => true do |t|
        t.column :title, :string, :limit => 255
        t.column :body, :text
      end
      Page.create_versioned_table
      Page.reset_column_information
    end
    def self.down
      drop_table :tepee_pages
      Page.drop_versioned_table
    end
  end

end

Tepee::Box = Sandbox.safe
Tepee::Box.load "support/support.rb"
Tepee::Box.ref Tepee::Models::Page
Tepee::Box.ref Web
Tepee::Box.ref Jungle
Tepee::Box.import URI::HTTP
Tepee::Box.import OpenURI::Meta
%w(CGI Time Hpricot HashWithIndifferentAccess
   PP JSON YAML OpenStruct Sandbox).each { |klass| Tepee::Box.import Kernel.const_get(klass) }
Tepee::Box.load "support/webdev.rb"

module Tepee::Controllers
  class Index < R '/'
    def get
      redirect Show, 'home'
    end
  end

  class Show < R '/(\w+)', '/(\w+)/(\d+)'
    def get page_name, version = nil
      redirect(Edit, page_name, 1) and return unless @page = Page.find_by_title(page_name)
      @version = (version.nil? or version == @page.version.to_s) ? @page : @page.versions.find_by_version(version)
      @cgi_parameters = CGI::parse(@env.REQUEST_URI.split('?')[-1])
      render :show
    end
  end

  class Edit < R '/(\w+)/edit', '/(\w+)/(\d+)/edit' 
    def get page_name, version = nil
      @page = Page.find_or_create_by_title(page_name)
      @page = @page.versions.find_by_version(version) unless version.nil? or version == @page.version.to_s
      render :edit
    end
    
    def post page_name
      Page.find_or_create_by_title(page_name).update_attributes :body => input.post_body and redirect Show, page_name
    end
  end
    
  class Source < R '/(\w+)/source', '/(\w+)/(\d+)/source' 
    def get page_name, version = nil
      @page = Page.find_by_title(page_name)
      @version = (version.nil? or version == @page.version.to_s) ? @page : @page.versions.find_by_version(version)
      @no_layout=true
      render :source
    end
  end

  class Versions < R '/(\w+)/versions'
    def get page_name
      @page = Page.find_or_create_by_title(page_name)
      @versions = @page.versions
      render :versions
    end
  end

  class List < R '/all/list'
    def get
      @pages = (Page.find :all, :order => 'title').reject { |p| p.title =~ /^private/ }
      render :list
    end
  end

  class Stylesheet < R '/css/tepee.css'
    def get
      @headers['Content-Type'] = 'text/css'
      File.read(__FILE__).gsub(/.*__END__/m, '')
    end
  end

  class Editor < R '/chunky/bacon/editor'
    def get
      @no_layout = true
      render :edit_code
    end
  end

  class Static < R '(/static/.+)'
    MIME_TYPES = {'.css' => 'text/css', '.js' => 'text/javascript', 
                  '.jpg' => 'image/jpeg', '.png' => 'image/png'}
    PATH = File.expand_path('.')

    def get(path)
      @headers['Content-Type'] = MIME_TYPES[path[/\.\w+$/, 0]] || "text/plain"
      if path.include? '..' || Path.expand_path(PATH+path) !~ /^#{PATH}/
        "404 - Invalid path" 
      else
        @headers['X-Sendfile'] = PATH+path
      end
    end
  end
end

module Tepee::Views
  def layout
    unless @no_layout
      html do
        head do
          title 'tippy tippy tepee'
          link :href=>R(Stylesheet), :rel=>'stylesheet', :type=>'text/css' 
        end
        style <<-END, :type => 'text/css'
          body {
            font-family: verdana, arial, sans-serif;
            min-width: 900px;
            background:#d7d7d7;
            text-align: center;
          }
          #doc {
            width: 900px;
            background:#ffffff;
            margin-left: auto;
            margin-right: auto;
            text-align: left;
          }
          h1, h2, h3, h4, h5 {
            font-weight: normal;
          }
          p.actions a {
            margin-right: 6px;
          }
        END
        body do
          div :id=>'doc' do
            p do
              small do
                span "welcome to " ; a 'tepee', :href => "http://code.whytheluckystiff.net/svn/camping/trunk/examples/tepee.rb"
                span '. go ' ;       a 'home',  :href => R(Show, 'home')
                span '. list all ' ; a 'pages', :href => R(List)
              end
            end
            div.content do
              self << yield
            end
          end
        end
      end
    else
      self << yield
    end
  end

  def _show_error(box)
    if @boxx
      line_no = (@boxx.to_s.scan(/(\d+)/).flatten[1] || "1").to_i - @line_zero + 1
      b { div.error! @boxx } #.to_s.gsub(/.eval.:\d+:/, '')
      pre.plain do
        @version.body.split("\n").each_with_index do |line, index|
          n = index+1
          text = n.to_s + ': ' + CGI::escapeHTML(line)
          self << div.highlight { text } if (n == line_no)
          self << text if (n != line_no)
        end
      end
    end
  end

  def _show_actions
    small do 
      span.actions do 
        text "Version #{@version.version} "
        text "(current) " if @version.version == @page.version
        a '«older',   :href => R(Show, @version.title, @version.version-1) unless @version.version == 1 
        a 'newer»',   :href => R(Show, @version.title, @version.version+1) unless @version.version == @page.version 
        a 'current',  :href => R(Show, @version.title)                     unless @version.version == @page.version 
        a 'versions', :href => R(Versions, @page.title) 
      end
    end
  end

  def show
    m = _markup @version.body
    unless @no_layout
      _button 'edit', R(Edit, @version.title, @version.version), { :style=>'float: right; margin: 0 0 5px 5px;', :accesskey => 'e' } if (@version.version == @page.version)

      h1 @page.title
      div { m }

      _show_error(@boxx)
      _button 'edit', R(Edit, @version.title, @version.version), { :style=>'float: right; margin: 5px 0 0 5px;' } if (@version.version == @page.version && @version.body && @version.body.size > 20)
      p { _show_actions }
    else
      self << m
    end
  end

  def source
    self << @version.body
  end

  def edit
    h1 @page.title 
    form :method => 'post', :action => R(Edit, @page.title) do
      input :type => 'submit', :value=>'save', :onclick=>'copyCode()'
      p do
        iframe :id=>'codepress', :name=>'codepress', :src=>'/chunky/bacon/editor', :width=>850, :height=>400
        br
        textarea @page.body, :id=>'codepress-onload', :name => 'post_body', :lang=>'ruby'
      end
      script 'function copyCode() { 
              var txt = document.getElementsByName("post_body")[0];
              txt.value = CodePress.getCode(); } ', :type => 'text/javascript'
      input :type => 'submit', :value=>'save', :onclick=>'copyCode()' 
    end
    _button 'cancel', R(Show, @page.title, @page.version) 
    a 'syntax', :href => 'http://pub.cozmixng.org/~the-rwiki/?cmd=view;name=ERbMemo.en', :target=>'_blank'
  end

  def edit_code
    html do
      head do
        link :href=>'/static/codepress.css', :rel=>'stylesheet', :type=>'text/css'
        link :href=>'/static/languages/codepress-ruby.css', :rel=>'stylesheet', 
             :type=>'text/css', :id=>'cp-lang-style'
        script :type=>'text/javascript', :src=>'/static/codepress.js'
        script :type=>'text/javascript', :src=>'/static/languages/codepress-ruby.js'
        script "CodePress.language = 'ruby';", :type=>'text/javascript'
      end
      body :id=>'ffedt' do
        pre :id=>'ieedt' do
        end
      end
    end
  end
  
  def list
    h1 'all pages'
    ul { @pages.each { |p| 
      li { a(p.title, :href => R(Show, p.title)) + " [" + a("edit", :href => R(Edit, p.title)) + "]" }
    } }
  end

  def versions
    h1 @page.title
    ul do
      @versions.each do |page|
        li do
          span page.version
          _button 'show', R(Show, page.title, page.version)
          _button 'edit', R(Edit, page.title, page.version)
        end
      end
    end
  end

  def _button(text, href, options={})
    form :method=>:get, :action=>href do
      opts = {:type=>'submit', :name=>'submit', :value=>text}.merge(options)
      input.button opts
    end
  end

  def _markup body
    return '' if body.blank?
    body.gsub!(Tepee::Models::Page::PAGE_LINK) do
      page = title = $1
      title = $2 unless $2.empty?
      page = page.gsub /\W/, '_'
      if Tepee::Models::Page.find(:all, :select => 'title').collect { |p| p.title }.include?(page)
        %Q{<a href="#{self/R(Show, page)}">#{title}</a>}
      else
        %Q{<span>#{title}<a href="#{self/R(Edit, page, 1)}">?</a></span>}
      end
    end
    _eval(body)
  end

  def _dump *var
    var.map { |v| "Marshal.load(#{Marshal.dump(v).dump})" }.join(',')
  end

  def _eval str
    @no_layout = false
    @boxx = nil
    @line_zero = 0
    begin
      str.gsub!(/^@\s+([\w\-]+):\s+(.+)$/) do
        @headers[$1] = $2.strip; ''
      end
      if @headers['Content-Type'] != 'text/html'
        @no_layout = true
      end
      code = %{
        instance_vars = {
          :env => #{_dump(@env)}, :input => #{_dump(@input)}, 
          :args => #{_dump(@cgi_parameters)}, 
          :session_id => #{_dump(@cookies.camping_sid)} 
        }
        
        doc = Markaby::Builder.new(instance_vars) do
          def puts(txt); self << txt; end
          ERbLight.new(#{str.dump}).result(binding)
        end.to_s
        
        meth = instance_vars[:args]['method']
        
        if meth.empty?
          doc
        else
          args = OpenStruct.new(instance_vars[:args])
          #{@page.title.gsub(/^./) {|c| c.upcase} }.send(meth[0], args)
        end
      }
      @line_zero = Tepee::Box.eval(%{__LINE__}) + code.count("\n") # FIXME
      str = Tepee::Box.eval code, :timeout => 10
    rescue Sandbox::Exception => @boxx
      "Caught Sandbox::Exception!"
    end
    # RedCloth.new(str, [ :hard_breaks ]).to_html
  end
end

def Tepee.create
  Tepee::Models.create_schema :assume => (Tepee::Models::Page.table_exists? ? 1.0 : 0.0)
  Tepee::Models::Session.create_schema
end

if __FILE__ == $0
  require 'mongrel/camping'
  Tepee::Models::Base.establish_connection :adapter => 'sqlite3', :database => ENV['HOME'] + '/.camping.db'
  Tepee::Models::Base.logger = Logger.new('tepee.log')
  Tepee::Models::Base.threaded_connections=false
  
  s = Mongrel::Camping.start('0.0.0.0', 3300, '/', Tepee)
  s.run.join
end

__END__
/** focus **/
/*
a:hover:active {
  color: #10bae0;
}

a:not(:hover):active {
  color: #0000ff;
}

*:focus {
  -moz-outline: 2px solid #10bae0 !important;
  -moz-outline-offset: 1px !important;
  -moz-outline-radius: 3px !important;
}

button:focus,
input[type="reset"]:focus,
input[type="button"]:focus,
input[type="submit"]:focus,
input[type="file"] > input[type="button"]:focus {
  -moz-outline-radius: 5px !important;
}

button:focus::-moz-focus-inner {
  border-color: transparent !important;
}

button::-moz-focus-inner,
input[type="reset"]::-moz-focus-inner,
input[type="button"]::-moz-focus-inner,
input[type="submit"]::-moz-focus-inner,
input[type="file"] > input[type="button"]::-moz-focus-inner {
  border: 1px dotted transparent !important;
}
textarea:focus, button:focus, select:focus, input:focus {
  -moz-outline-offset: -1px !important;
}
input[type="radio"]:focus {
  -moz-outline-radius: 12px;
  -moz-outline-offset: 0px !important;
}
a:focus {
  -moz-outline-offset: 0px !important;
}
*/
form { display: inline; }

/** Gradient **/
small, pre, textarea, textfield, button, input, select, blockquote {
   color: #4B4B4C !important;
   background-image: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAAeCAMAAAAxfD/2AAAABGdBTUEAAK/INwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAAtUExURfT09PLy8vHx8fv7+/j4+PX19fn5+fr6+vf39/z8/Pb29vPz8/39/f7+/v///0c8Y4oAAAA5SURBVHjaXMZJDgAgCMDAuouA/3+uHPRiMmlKzmhCFRorLOakVnpnDEpBBDHM8ODs/bz372+PAAMAXIQCfD6uIDsAAAAASUVORK5CYII=) !important;
   background-color: #FFF !important;
   background-repeat: repeat-x !important;
   border: 1px solid #CCC !important;
}

pre.plain {
   border: 0px
}

div.highlight {
   color: #FF0066
}

div#error { background-color: #FF0066; color: #FFFFFF !important; }

span.actions a {
    margin-right: 10px;
}

/* WIKI CONTENT STYLES */
.content {
    padding: 0px 25px 5px 25px;
    min-height: 300px;
}

button, input { margin: 3px; }

