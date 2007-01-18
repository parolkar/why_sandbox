#!/usr/local/bin/ruby
# Tiny Tiny eRuby 
# $Id: erbl.rb,v 1.5 1999/08/03 07:25:32 mas Exp $
# $Author: mas $
# Copyright (c) 1999 Masatoshi SEKI

class ARegexpAbstract
  def initialize
    @greedy = true
    @min = 1
    @max = 1
  end

  def match_one(list, start=0)
    []
  end

  def match_set(list, currSet)
    fwdSet = []
    currSet.each do |start|
      self.match_one(list, start).each do |fwd|
	fwdSet.push(fwd)
      end
    end
    fwdSet.sort.uniq
  end

  # 評価するリストと先頭のindexを設定する
  def match(list, idx=0)
    fwd_set = []      	# fwd_set は移動先の集合
    # 0回とマッチするなら今のインデックスを返す
    fwd_set.push(idx) if matchEmpty?
    
    # min回調べる
    next_set = [idx]	# next_set は新たに見つかった移動先の集合
    @min.times do 
      next_set = self.match_set(list, next_set).uniq.sort
    end
    fwd_set = fwd_set + next_set
    
    # 何度もマッチ?
    if matchMany?
      # 新たに見つかった移動先がある間
      # 次の移動先の集合を求める
      while(next_set.size > 0)
	tmp = next_set
	next_set = []
	self.match_set(list, tmp).each do |i|
	  unless fwd_set.include? i
	    fwd_set.push(i)
	    next_set.push(i)
	  end
	end
      end
    end

    self.fwd_list(fwd_set)
  end

  # 繰返し回数を設定
  # max == -1 は上限の指定がないことを意味する
  def repeat(min, max)
    @min = min
    max = -1 unless max
    @max = max
    self
  end
  def matchEmpty?
    (@min == 0)
  end
  def matchMany?
    (@max < 0)
  end
  
  # 
  def greedy(v)
    @greedy = v
    self
  end
  def fwd_list(list)
    list = list.uniq.sort
    unless @greedy
      list.reverse!
    end
    list
  end
end

class ARegexpProc < ARegexpAbstract
  def initialize(proc)
    @proc = proc
    @neg = false
    super()
  end

  def match_one(list, idx=0)
    if (list.size > idx) and (self.test(list[idx]))
      [idx + 1]
    else
      []
    end
  end

  def test(v)
    result = @proc.call(v)
    if @neg 
      not result
    else
      result
    end
  end

  def negate(neg=true)
    @neg = neg
    self
  end
end

class ARegexpEq < ARegexpProc
  def initialize(r)
    p = proc{|v| v==r}
    super(p)
  end
end

class ARegexpInclude < ARegexpProc
  def initialize(list)
    p = proc{|v| list.include?(v)}
    super(p)
  end
end

class ARegexpSeq < ARegexpAbstract
  def initialize(patterns = [])
    @seq = patterns
    super()
  end
  def add(pat)
    @seq.push(pat)
  end

  # 始点 から 移動先の集合 を求める
  def match_one(list, idx=0)
    aSet = []
    i = seq_match(list, idx, 0)
    aSet.push(i) if i
    aSet.uniq
  end

  def seq_match(list, idx, seqIdx)
    if seqIdx >= @seq.size
      return idx
    end
    @seq[seqIdx].match(list, idx).reverse_each do |fwd|
      idx = seq_match(list, fwd, seqIdx + 1)
      return idx if idx
    end
    nil
  end
end

class ARegexpAlt < ARegexpAbstract
  def initialize(patterns = [])
    @alt = patterns
    super()
  end
  def add(pat)
    @alt.push(pat)
  end

  # list の 始点の集合 から 移動先の集合 を求める
  def match_one(list, idx=0)
    aSet = []
    @alt.each do |re|
      aSet.push(re.match(list, idx))
    end
    aSet.flatten.uniq.sort
  end
end

class ARegexpAny < ARegexpAbstract
  def match_one(list, idx=0)
    if list.size > idx
      [idx + 1]
    else
      []
    end
  end
end

class ARegexpFirst < ARegexpAbstract
  def match_one(list, idx=0)
    if idx == 0
      [idx]
    else
      []
    end
  end

  def match(list, idx=0)
    match_one(list, idx)
  end
end

class ARegexpLast < ARegexpAbstract
  def match_one(list, idx=0)
    if list.size == idx
      [idx]
    else
      []
    end
  end

  def match(list, idx=0)
    match_one(list, idx)
  end
end

class ARegexp < ARegexpSeq
  def initialize(patterns = [])
    super(patterns)
  end
  undef repeat
  undef greedy

  def shallow_copy
    ARegexp.new(@seq)
  end
  alias cp shallow_copy

  def match(list, idx=0)
    @list = list
    @matching_offset = Array.new(@seq.size + 1)
    @matching_offset[0] = idx
    v = fwd_list(match_one(list, idx))
    @matching_offset = nil if v.size == 0
    v
  end

  def seq_match(list, idx, seqIdx)
    @matching_offset[seqIdx] = idx
    super(list, idx, seqIdx)
  end
  attr :matching_offset

  def matching_data
    return [] unless @matching_offset
    curr = @matching_offset[0]
    @matching_offset[1..-1].collect { |fwd|
      len = fwd - curr
      if len > 0
	v = @list[curr, len]
      else
	v = []
      end
      curr = fwd
      v
    }
  end

  def =~(list)
    list.each_index do |idx|
      fwd = self.match(list, idx)
      if fwd.size > 0
	return idx
      end
    end
    nil
  end

  def gsub(list)
    v = []
    idx = 0
    while idx < list.size
      fwd = self.match(list, idx)
      if fwd.size == 0
	v.push(list[idx])
	idx += 1
      else
	fwdidx = fwd.max
	len = fwdidx - idx
	if len == 0
	  s = yield([])
	  v += s if s 
	  idx += 1
	else
	  s = yield(list[idx, len])
	  v += s if s 
	  idx = fwdidx
	end
      end
    end
    v
  end

  def scan(list)
    idx = 0
    while idx < list.size
      fwd = self.match(list, idx)
      if fwd.size == 0
	idx += 1
      else
	fwdidx = fwd.max
	len = fwdidx - idx
	if len == 0
	  yield([])
	  idx += 1
	else
	  yield(list[idx, len])
	  idx = fwdidx
	end
      end
    end
    self
  end

  def sub(list)
    v = list.dup
    idx = 0
    while idx < list.size
      fwd = self.match(list, idx)
      if fwd.size == 0
	idx += 1
      else
	fwdidx = fwd.max
	len = fwdidx - idx
	if len <= 0
	  v[idx, 0] = yield([])
	else
	  v[idx, len] = yield(list[idx, len])
	end
	return v
      end
    end
    v
  end
end

class ERbLight
  Revision = '$Date: 1999/08/03 07:25:32 $'

class << self
  def version
    "erb.rb [#{ERb::Revision.split[1]}]"
  end

  def pre_compile(s)
    list = []
    s.each_line do |line|
      line.gsub!(/^(%[#=]?)\s+(.+)/) { "\n<#$1\n#$2\n%>\n" } or
      line.gsub!(/(<%%)|(%%>)|(<%=)|(<%#)|(<%)|(%>)/) do |m|
	"\n#{m}\n"
      end
      list += line.split("\n")
      list.push("\n")
    end

    escape = @escape.cp
    list = escape.gsub(list) { 
      case escape.matching_data[0].join
      when '<%%'
	['<', '%']
      when '%%>'
	['%', '>']
      end
    }    	
  end

  def compile(s)
    cmdlist = []
    str_table = []
    list = pre_compile(s)

    cmdlist.push('begin')
    cmdlist.push('_erbout = ""')
    cmdlist.push('$SAFE = @safe_level') if @safe_level

    head = @head.cp
    list = head.sub(list) {
      match = head.matching_data
      match[1].join.each do |line|
	cmdlist.push("_erbout.concat #{line.dump}")
      end
      nil
    }
    compile = @compile.cp
    list = compile.gsub(list) {
      match = compile.matching_data
      content = match[1].join
      case match[0].join
      when '<%'
	cmdlist.push(content)
      when '<%='
	cmdlist.push("_erbout.concat((#{content}).to_s)")
      when '<%#'
=begin
for ruby 1.3 ...
	cmdlist.push('=begin')
	cmdlist.push(content)
	cmdlist.push('=end')
=end
	cmdlist.push("# #{content.dump}")
      end
      match[3].join.each do |line|
	cmdlist.push("_erbout.concat #{line.dump}")
      end
      nil
    }
    list.join.each do |line|
      cmdlist.push("_erbout.concat #{line.dump}")
    end
    cmdlist.push('ensure')
    cmdlist.push('_erbout')
    cmdlist.push('end')
    cmdlist.join("\n")
  end

  def setup_compiler
    first = ARegexpFirst.new
    stag = ARegexpInclude.new(['<%=', '<%#', '<%'])
    notstag = stag.dup.negate.repeat(0, nil).greedy(true)
    etag = ARegexpEq.new('%>')
    any = ARegexpAny.new.repeat(0, nil).greedy(false)
    @head = ARegexp.new([first, notstag]);
    @compile = ARegexp.new([stag, any, etag, notstag])
    escape = ARegexpInclude.new(['<%%', '%%>'])
    @escape = ARegexp.new([escape])
  end
end

  def initialize(str, safe_level=nil)
    @safe_level = safe_level
    @src = ERbLight.compile(str)
  end
  attr :src

  setup_compiler

  def run(b=TOPLEVEL_BINDING)
    print self.result(b)
  end

  def result(b=TOPLEVEL_BINDING)
    if @safe_level
      th = Thread.start { 
	eval(@src, b)
      }
      return th.value
    else
      return eval(@src, b)
    end
  end
end

module Kernel
  class << self
    alias_method :blank_slate_method_added, :method_added

    # Detect method additions to Kernel and remove them in the
    # BlankSlate class.
    def method_added(name)
      blank_slate_method_added(name)
      return if self != Kernel
      Builder::BlankSlate.hide(name)
    end
  end
end

class Object
  class << self
    alias_method :blank_slate_method_added, :method_added

    # Detect method additions to Object and remove them in the
    # BlankSlate class.
    def method_added(name)
      blank_slate_method_added(name)
      return if self != Object
      Builder::BlankSlate.hide(name)
    end
  end
end
module Builder

  # BlankSlate provides an abstract base class with no predefined
  # methods (except for <tt>\_\_send__</tt> and <tt>\_\_id__</tt>).
  # BlankSlate is useful as a base class when writing classes that
  # depend upon <tt>method_missing</tt> (e.g. dynamic proxies).
  class BlankSlate
    class << self

      # Hide the method named +name+ in the BlankSlate class.  Don't
      # hide +instance_eval+ or any method beginning with "__".
      def hide(name)
	undef_method name if
	  instance_methods.include?(name.to_s) and
	  name !~ /^(__|instance_eval)/
      end
    end

    instance_methods.each { |m| hide(m) }
  end

  module XChar # :nodoc:

    # See
    # http://intertwingly.net/stories/2004/04/14/i18n.html#CleaningWindows
    # for details.
    CP1252 = {			# :nodoc:
      128 => 8364,		# euro sign
      130 => 8218,		# single low-9 quotation mark
      131 =>  402,		# latin small letter f with hook
      132 => 8222,		# double low-9 quotation mark
      133 => 8230,		# horizontal ellipsis
      134 => 8224,		# dagger
      135 => 8225,		# double dagger
      136 =>  710,		# modifier letter circumflex accent
      137 => 8240,		# per mille sign
      138 =>  352,		# latin capital letter s with caron
      139 => 8249,		# single left-pointing angle quotation mark
      140 =>  338,		# latin capital ligature oe
      142 =>  381,		# latin capital letter z with caron
      145 => 8216,		# left single quotation mark
      146 => 8217,		# right single quotation mark
      147 => 8220,		# left double quotation mark
      148 => 8221,		# right double quotation mark
      149 => 8226,		# bullet
      150 => 8211,		# en dash
      151 => 8212,		# em dash
      152 =>  732,		# small tilde
      153 => 8482,		# trade mark sign
      154 =>  353,		# latin small letter s with caron
      155 => 8250,		# single right-pointing angle quotation mark
      156 =>  339,		# latin small ligature oe
      158 =>  382,		# latin small letter z with caron
      159 =>  376,		# latin capital letter y with diaeresis
    }

    # See http://www.w3.org/TR/REC-xml/#dt-chardata for details.
    PREDEFINED = {
      38 => '&amp;',		# ampersand
      60 => '&lt;',		# left angle bracket
      62 => '&gt;',		# right angle bracket
    }

    # See http://www.w3.org/TR/REC-xml/#charsets for details.
    VALID = [
      [0x9, 0xA, 0xD],
      (0x20..0xD7FF), 
      (0xE000..0xFFFD),
      (0x10000..0x10FFFF)
    ]
  end

end

######################################################################
# Enhance the Fixnum class with a XML escaped character conversion.
#
class Fixnum
  XChar = Builder::XChar if ! defined?(XChar)

  # XML escaped version of chr
  def xchr
    n = XChar::CP1252[self] || self
    n = 42 unless XChar::VALID.find {|range| range.include? n}
    XChar::PREDEFINED[n] or (n<128 ? n.chr : "&##{n};")
  end
end


######################################################################
# Enhance the String class with a XML escaped character version of
# to_s.
#
class String
  # XML escaped version of to_s
  def to_xs
    unpack('U*').map {|n| n.xchr}.join # ASCII, UTF-8
  rescue
    unpack('C*').map {|n| n.xchr}.join # ISO-8859-1, WIN-1252
  end
end

module Builder

  # Generic error for builder
  class IllegalBlockError < RuntimeError; end

  # XmlBase is a base class for building XML builders.  See
  # Builder::XmlMarkup and Builder::XmlEvents for examples.
  class XmlBase < BlankSlate

    # Create an XML markup builder.
    #
    # out::     Object receiving the markup.  +out+ must respond to
    #           <tt><<</tt>.
    # indent::  Number of spaces used for indentation (0 implies no
    #           indentation and no line breaks).
    # initial:: Level of initial indentation.
    #
    def initialize(indent=0, initial=0)
      @indent = indent
      @level  = initial
    end
    
    # Create a tag named +sym+.  Other than the first argument which
    # is the tag name, the arguements are the same as the tags
    # implemented via <tt>method_missing</tt>.
    def tag!(sym, *args, &block)
      self.__send__(sym, *args, &block)
    end

    # Create XML markup based on the name of the method.  This method
    # is never invoked directly, but is called for each markup method
    # in the markup block.
    def method_missing(sym, *args, &block)
      text = nil
      attrs = nil
      sym = "#{sym}:#{args.shift}" if args.first.kind_of?(Symbol)
      args.each do |arg|
	case arg
	when Hash
	  attrs ||= {}
	  attrs.merge!(arg)
	else
	  text ||= ''
	  text << arg.to_s
	end
      end
      if block
	unless text.nil?
	  raise ArgumentError, "XmlMarkup cannot mix a text argument with a block"
	end
	_capture_outer_self(block) if @self.nil?
	_indent
	_start_tag(sym, attrs)
	_newline
	_nested_structures(block)
	_indent
	_end_tag(sym)
	_newline
      elsif text.nil?
	_indent
	_start_tag(sym, attrs, true)
	_newline
      else
	_indent
	_start_tag(sym, attrs)
	text! text
	_end_tag(sym)
	_newline
      end
      @target
    end

    # Append text to the output target.  Escape any markup.  May be
    # used within the markup brakets as:
    #
    #   builder.p { |b| b.br; b.text! "HI" }   #=>  <p><br/>HI</p>
    def text!(text)
      _text(_escape(text))
    end
    
    # Append text to the output target without escaping any markup.
    # May be used within the markup brakets as:
    #
    #   builder.p { |x| x << "<br/>HI" }   #=>  <p><br/>HI</p>
    #
    # This is useful when using non-builder enabled software that
    # generates strings.  Just insert the string directly into the
    # builder without changing the inserted markup.
    #
    # It is also useful for stacking builder objects.  Builders only
    # use <tt><<</tt> to append to the target, so by supporting this
    # method/operation builders can use other builders as their
    # targets.
    def <<(text)
      _text(text)
    end
    
    # For some reason, nil? is sent to the XmlMarkup object.  If nil?
    # is not defined and method_missing is invoked, some strange kind
    # of recursion happens.  Since nil? won't ever be an XML tag, it
    # is pretty safe to define it here. (Note: this is an example of
    # cargo cult programming,
    # cf. http://fishbowl.pastiche.org/2004/10/13/cargo_cult_programming).
    def nil?
      false
    end

    private
    
    def _escape(text)
      text.to_xs
    end

    def _escape_quote(text)
      _escape(text).gsub(%r{"}, '&quot;')  # " WART
    end

    def _capture_outer_self(block)
      @self = eval("self", block)
    end
    
    def _newline
      return if @indent == 0
      text! "\n"
    end
    
    def _indent
      return if @indent == 0 || @level == 0
      text!(" " * (@level * @indent))
    end
    
    def _nested_structures(block)
      @level += 1
      block.call(self)
    ensure
      @level -= 1
    end
  end
  # Create XML markup easily.  All (well, almost all) methods sent to
  # an XmlMarkup object will be translated to the equivalent XML
  # markup.  Any method with a block will be treated as an XML markup
  # tag with nested markup in the block.
  #
  # Examples will demonstrate this easier than words.  In the
  # following, +xm+ is an +XmlMarkup+ object.
  #
  #   xm.em("emphasized")             # => <em>emphasized</em>
  #   xm.em { xmm.b("emp & bold") }   # => <em><b>emph &amp; bold</b></em>
  #   xm.a("A Link", "href"=>"http://onestepback.org")
  #                                   # => <a href="http://onestepback.org">A Link</a>
  #   xm.div { br }                    # => <div><br/></div>
  #   xm.target("name"=>"compile", "option"=>"fast")
  #                                   # => <target option="fast" name="compile"\>
  #                                   # NOTE: order of attributes is not specified.
  #
  #   xm.instruct!                   # <?xml version="1.0" encoding="UTF-8"?>
  #   xm.html {                      # <html>
  #     xm.head {                    #   <head>
  #       xm.title("History")        #     <title>History</title>
  #     }                            #   </head>
  #     xm.body {                    #   <body>
  #       xm.comment! "HI"           #     <!-- HI -->
  #       xm.h1("Header")            #     <h1>Header</h1>
  #       xm.p("paragraph")          #     <p>paragraph</p>
  #     }                            #   </body>
  #   }                              # </html>
  #
  # == Notes:
  #
  # * The order that attributes are inserted in markup tags is
  #   undefined. 
  #
  # * Sometimes you wish to insert text without enclosing tags.  Use
  #   the <tt>text!</tt> method to accomplish this.
  #
  #   Example:
  #
  #     xm.div {                          # <div>
  #       xm.text! "line"; xm.br          #   line<br/>
  #       xm.text! "another line"; xmbr   #    another line<br/>
  #     }                                 # </div>
  #
  # * The special XML characters <, >, and & are converted to &lt;,
  #   &gt; and &amp; automatically.  Use the <tt><<</tt> operation to
  #   insert text without modification.
  #
  # * Sometimes tags use special characters not allowed in ruby
  #   identifiers.  Use the <tt>tag!</tt> method to handle these
  #   cases.
  #
  #   Example:
  #
  #     xml.tag!("SOAP:Envelope") { ... }
  #
  #   will produce ...
  #
  #     <SOAP:Envelope> ... </SOAP:Envelope>"
  #
  #   <tt>tag!</tt> will also take text and attribute arguments (after
  #   the tag name) like normal markup methods.  (But see the next
  #   bullet item for a better way to handle XML namespaces).
  #   
  # * Direct support for XML namespaces is now available.  If the
  #   first argument to a tag call is a symbol, it will be joined to
  #   the tag to produce a namespace:tag combination.  It is easier to
  #   show this than describe it.
  #
  #     xml.SOAP :Envelope do ... end
  #
  #   Just put a space before the colon in a namespace to produce the
  #   right form for builder (e.g. "<tt>SOAP:Envelope</tt>" =>
  #   "<tt>xml.SOAP :Envelope</tt>")
  #
  # * XmlMarkup builds the markup in any object (called a _target_)
  #   that accepts the <tt><<</tt> method.  If no target is given,
  #   then XmlMarkup defaults to a string target.
  # 
  #   Examples:
  #
  #     xm = Builder::XmlMarkup.new
  #     result = xm.title("yada")
  #     # result is a string containing the markup.
  #
  #     buffer = ""
  #     xm = Builder::XmlMarkup.new(buffer)
  #     # The markup is appended to buffer (using <<)
  #
  #     xm = Builder::XmlMarkup.new(STDOUT)
  #     # The markup is written to STDOUT (using <<)
  #
  #     xm = Builder::XmlMarkup.new
  #     x2 = Builder::XmlMarkup.new(:target=>xm)
  #     # Markup written to +x2+ will be send to +xm+.
  #   
  # * Indentation is enabled by providing the number of spaces to
  #   indent for each level as a second argument to XmlBuilder.new.
  #   Initial indentation may be specified using a third parameter.
  #
  #   Example:
  #
  #     xm = Builder.new(:ident=>2)
  #     # xm will produce nicely formatted and indented XML.
  #  
  #     xm = Builder.new(:indent=>2, :margin=>4)
  #     # xm will produce nicely formatted and indented XML with 2
  #     # spaces per indent and an over all indentation level of 4.
  #
  #     builder = Builder::XmlMarkup.new(:target=>$stdout, :indent=>2)
  #     builder.name { |b| b.first("Jim"); b.last("Weirich) }
  #     # prints:
  #     #     <name>
  #     #       <first>Jim</first>
  #     #       <last>Weirich</last>
  #     #     </name>
  #
  # * The instance_eval implementation which forces self to refer to
  #   the message receiver as self is now obsolete.  We now use normal
  #   block calls to execute the markup block.  This means that all
  #   markup methods must now be explicitly send to the xml builder.
  #   For instance, instead of
  #
  #      xml.div { strong("text") }
  #
  #   you need to write:
  #
  #      xml.div { xml.strong("text") }
  #
  #   Although more verbose, the subtle change in semantics within the
  #   block was found to be prone to error.  To make this change a
  #   little less cumbersome, the markup block now gets the markup
  #   object sent as an argument, allowing you to use a shorter alias
  #   within the block.
  #
  #   For example:
  #
  #     xml_builder = Builder::XmlMarkup.new
  #     xml_builder.div { |xml|
  #       xml.stong("text")
  #     }
  #
  class XmlMarkup < XmlBase

    # Create an XML markup builder.  Parameters are specified by an
    # option hash.
    #
    # :target=><em>target_object</em>::
    #    Object receiving the markup.  +out+ must respond to the
    #    <tt><<</tt> operator.  The default is a plain string target.
    #    
    # :indent=><em>indentation</em>::
    #    Number of spaces used for indentation.  The default is no
    #    indentation and no line breaks.
    #    
    # :margin=><em>initial_indentation_level</em>::
    #    Amount of initial indentation (specified in levels, not
    #    spaces).
    #    
    # :escape_attrs=><b>OBSOLETE</em>::
    #    The :escape_attrs option is no longer supported by builder
    #    (and will be quietly ignored).  String attribute values are
    #    now automatically escaped.  If you need unescaped attribute
    #    values (perhaps you are using entities in the attribute
    #    values), then give the value as a Symbol.  This allows much
    #    finer control over escaping attribute values.
    #    
    def initialize(options={})
      indent = options[:indent] || 0
      margin = options[:margin] || 0
      super(indent, margin)
      @target = options[:target] || ""
    end
    
    # Return the target of the builder.
    def target!
      @target
    end

    def comment!(comment_text)
      _ensure_no_block block_given?
      _special("<!-- ", " -->", comment_text, nil)
    end

    # Insert an XML declaration into the XML markup.
    #
    # For example:
    #
    #   xml.declare! :ELEMENT, :blah, "yada"
    #       # => <!ELEMENT blah "yada">
    def declare!(inst, *args, &block)
      _indent
      @target << "<!#{inst}"
      args.each do |arg|
	case arg
	when String
	  @target << %{ "#{arg}"}
	when Symbol
	  @target << " #{arg}"
	end
      end
      if block_given?
	@target << " ["
	_newline
	_nested_structures(block)
	@target << "]"
      end
      @target << ">"
      _newline
    end

    # Insert a processing instruction into the XML markup.  E.g.
    #
    # For example:
    #
    #    xml.instruct!
    #        #=> <?xml version="1.0" encoding="UTF-8"?>
    #    xml.instruct! :aaa, :bbb=>"ccc"
    #        #=> <?aaa bbb="ccc"?>
    #
    def instruct!(directive_tag=:xml, attrs={})
      _ensure_no_block block_given?
      if directive_tag == :xml
	a = { :version=>"1.0", :encoding=>"UTF-8" }
	attrs = a.merge attrs
      end
      _special(
	"<?#{directive_tag}",
	"?>",
	nil,
	attrs,
	[:version, :encoding, :standalone])
    end

    # Insert a CDATA section into the XML markup.
    #
    # For example:
    #
    #    xml.cdata!("text to be included in cdata")
    #        #=> <![CDATA[text to be included in cdata]]>
    #
    def cdata!(text)
      _ensure_no_block block_given?
      _special("<![CDATA[", "]]>", text, nil)
    end
    
    private

    # NOTE: All private methods of a builder object are prefixed when
    # a "_" character to avoid possible conflict with XML tag names.

    # Insert text directly in to the builder's target.
    def _text(text)
      @target << text
    end
    
    # Insert special instruction. 
    def _special(open, close, data=nil, attrs=nil, order=[])
      _indent
      @target << open
      @target << data if data
      _insert_attributes(attrs, order) if attrs
      @target << close
      _newline
    end

    # Start an XML tag.  If <tt>end_too</tt> is true, then the start
    # tag is also the end tag (e.g.  <br/>
    def _start_tag(sym, attrs, end_too=false)
      @target << "<#{sym}"
      _insert_attributes(attrs)
      @target << "/" if end_too
      @target << ">"
    end
    
    # Insert an ending tag.
    def _end_tag(sym)
      @target << "</#{sym}>"
    end

    # Insert the attributes (given in the hash).
    def _insert_attributes(attrs, order=[])
      return if attrs.nil?
      order.each do |k|
	v = attrs[k]
	@target << %{ #{k}="#{_attr_value(v)}"} if v
      end
      attrs.each do |k, v|
	@target << %{ #{k}="#{_attr_value(v)}"} unless order.member?(k)
      end
    end

    def _attr_value(value)
      case value
      when Symbol
	value.to_s
      else
	_escape_quote(value.to_s)
      end
    end

    def _ensure_no_block(got_block)
      if got_block
	fail IllegalBlockError,
	  "Blocks are not allowed on XML instructions"
      end
    end

  end

  # Create a series of SAX-like XML events (e.g. start_tag, end_tag)
  # from the markup code.  XmlEvent objects are used in a way similar
  # to XmlMarkup objects, except that a series of events are generated
  # and passed to a handler rather than generating character-based
  # markup.
  #
  # Usage:
  #   xe = Builder::XmlEvents.new(hander)
  #   xe.title("HI")    # Sends start_tag/end_tag/text messages to the handler.
  #
  # Indentation may also be selected by providing value for the
  # indentation size and initial indentation level.
  #
  #   xe = Builder::XmlEvents.new(handler, indent_size, initial_indent_level)
  #
  # == XML Event Handler
  #
  # The handler object must expect the following events.
  #
  # [<tt>start_tag(tag, attrs)</tt>]
  #     Announces that a new tag has been found.  +tag+ is the name of
  #     the tag and +attrs+ is a hash of attributes for the tag.
  #
  # [<tt>end_tag(tag)</tt>]
  #     Announces that an end tag for +tag+ has been found.
  #
  # [<tt>text(text)</tt>]
  #     Announces that a string of characters (+text+) has been found.
  #     A series of characters may be broken up into more than one
  #     +text+ call, so the client cannot assume that a single
  #     callback contains all the text data.
  #
  class XmlEvents < XmlMarkup
    def text!(text)
      @target.text(text)
    end

    def _start_tag(sym, attrs, end_too=false)
      @target.start_tag(sym, attrs)
      _end_tag(sym) if end_too
    end

    def _end_tag(sym)
      @target.end_tag(sym)
    end
  end
end

module Markaby
  VERSION = '0.5'

  class InvalidXhtmlError < Exception; end

  FORM_TAGS = [ :form, :input, :select, :textarea ]
  SELF_CLOSING_TAGS = [ :base, :meta, :link, :hr, :br, :param, :img, :area, :input, :col ]
  NO_PROXY = [ :hr, :br ]

  # Common sets of attributes.
  AttrCore = [:id, :class, :style, :title]
  AttrI18n = [:lang, 'xml:lang'.intern, :dir]
  AttrEvents = [:onclick, :ondblclick, :onmousedown, :onmouseup, :onmouseover, :onmousemove, 
      :onmouseout, :onkeypress, :onkeydown, :onkeyup]
  AttrFocus = [:accesskey, :tabindex, :onfocus, :onblur]
  AttrHAlign = [:align, :char, :charoff]
  AttrVAlign = [:valign]
  Attrs = AttrCore + AttrI18n + AttrEvents

  # All the tags and attributes from XHTML 1.0 Strict
  class XHTMLStrict
    class << self
      attr_accessor :tags, :tagset, :forms, :self_closing, :doctype
    end
    @doctype = ["-//W3C//DTD XHTML 1.0 Strict//EN", "DTD/xhtml1-strict.dtd"]
    @tagset = {
      :html => AttrI18n + [:id, :xmlns],
      :head => AttrI18n + [:id, :profile],
      :title => AttrI18n + [:id],
      :base => [:href, :id],
      :meta => AttrI18n + [:id, :http, :name, :content, :scheme, 'http-equiv'.intern],
      :link => Attrs + [:charset, :href, :hreflang, :type, :rel, :rev, :media],
      :style => AttrI18n + [:id, :type, :media, :title, 'xml:space'.intern],
      :script => [:id, :charset, :type, :src, :defer, 'xml:space'.intern],
      :noscript => Attrs,
      :body => Attrs + [:onload, :onunload],
      :div => Attrs,
      :p => Attrs,
      :ul => Attrs,
      :ol => Attrs,
      :li => Attrs,
      :dl => Attrs,
      :dt => Attrs,
      :dd => Attrs,
      :address => Attrs,
      :hr => Attrs,
      :pre => Attrs + ['xml:space'.intern],
      :blockquote => Attrs + [:cite],
      :ins => Attrs + [:cite, :datetime],
      :del => Attrs + [:cite, :datetime],
      :a => Attrs + AttrFocus + [:charset, :type, :name, :href, :hreflang, :rel, :rev, :shape, :coords],
      :span => Attrs,
      :bdo => AttrCore + AttrEvents + [:lang, 'xml:lang'.intern, :dir],
      :br => AttrCore,
      :em => Attrs,
      :strong => Attrs,
      :dfn => Attrs,
      :code => Attrs,
      :samp => Attrs,
      :kbd => Attrs,
      :var => Attrs,
      :cite => Attrs,
      :abbr => Attrs,
      :acronym => Attrs,
      :q => Attrs + [:cite],
      :sub => Attrs,
      :sup => Attrs,
      :tt => Attrs,
      :i => Attrs,
      :b => Attrs,
      :big => Attrs,
      :small => Attrs,
      :object => Attrs + [:declare, :classid, :codebase, :data, :type, :codetype, :archive, :standby, :height, :width, :usemap, :name, :tabindex],
      :param => [:id, :name, :value, :valuetype, :type],
      :img => Attrs + [:src, :alt, :longdesc, :height, :width, :usemap, :ismap],
      :map => AttrI18n + AttrEvents + [:id, :class, :style, :title, :name],
      :area => Attrs + AttrFocus + [:shape, :coords, :href, :nohref, :alt],
      :form => Attrs + [:action, :method, :enctype, :onsubmit, :onreset, :accept, :accept],
      :label => Attrs + [:for, :accesskey, :onfocus, :onblur],
      :input => Attrs + AttrFocus + [:type, :name, :value, :checked, :disabled, :readonly, :size, :maxlength, :src, :alt, :usemap, :onselect, :onchange, :accept],
      :select => Attrs + [:name, :size, :multiple, :disabled, :tabindex, :onfocus, :onblur, :onchange],
      :optgroup => Attrs + [:disabled, :label],
      :option => Attrs + [:selected, :disabled, :label, :value],
      :textarea => Attrs + AttrFocus + [:name, :rows, :cols, :disabled, :readonly, :onselect, :onchange],
      :fieldset => Attrs,
      :legend => Attrs + [:accesskey],
      :button => Attrs + AttrFocus + [:name, :value, :type, :disabled],
      :table => Attrs + [:summary, :width, :border, :frame, :rules, :cellspacing, :cellpadding],
      :caption => Attrs,
      :colgroup => Attrs + AttrHAlign + AttrVAlign + [:span, :width],
      :col => Attrs + AttrHAlign + AttrVAlign + [:span, :width],
      :thead => Attrs + AttrHAlign + AttrVAlign,
      :tfoot => Attrs + AttrHAlign + AttrVAlign,
      :tbody => Attrs + AttrHAlign + AttrVAlign,
      :tr => Attrs + AttrHAlign + AttrVAlign,
      :th => Attrs + AttrHAlign + AttrVAlign + [:abbr, :axis, :headers, :scope, :rowspan, :colspan],
      :td => Attrs + AttrHAlign + AttrVAlign + [:abbr, :axis, :headers, :scope, :rowspan, :colspan],
      :h1 => Attrs,
      :h2 => Attrs,
      :h3 => Attrs,
      :h4 => Attrs,
      :h5 => Attrs,
      :h6 => Attrs
    }

    @tags = @tagset.keys
    @forms = @tags & FORM_TAGS
    @self_closing = @tags & SELF_CLOSING_TAGS
  end

  # Additional tags found in XHTML 1.0 Transitional
  class XHTMLTransitional
    class << self
      attr_accessor :tags, :tagset, :forms, :self_closing, :doctype
    end
    @doctype = ["-//W3C//DTD XHTML 1.0 Transitional//EN", "DTD/xhtml1-transitional.dtd"]
    @tagset = XHTMLStrict.tagset.merge \
      :strike => Attrs,
      :center => Attrs,
      :dir => Attrs + [:compact], 
      :noframes => Attrs,
      :basefont => [:id, :size, :color, :face], 
      :u => Attrs,
      :menu => Attrs + [:compact], 
      :iframe => AttrCore + [:longdesc, :name, :src, :frameborder, :marginwidth, :marginheight, :scrolling, :align, :height, :width],
      :font => AttrCore + AttrI18n + [:size, :color, :face],
      :s => Attrs,
      :applet => AttrCore + [:codebase, :archive, :code, :object, :alt, :name, :width, :height, :align, :hspace, :vspace],
      :isindex => AttrCore + AttrI18n + [:prompt]

    # Additional attributes found in XHTML 1.0 Transitional
    { :script => [:language],
      :a => [:target],
      :td => [:bgcolor, :nowrap, :width, :height],
      :p => [:align],
      :h5 => [:align],
      :h3 => [:align],
      :li => [:type, :value],
      :div => [:align],
      :pre => [:width],
      :body => [:background, :bgcolor, :text, :link, :vlink, :alink],
      :ol => [:type, :compact, :start],
      :h4 => [:align],
      :h2 => [:align],
      :object => [:align, :border, :hspace, :vspace],
      :img => [:name, :align, :border, :hspace, :vspace],
      :link => [:target],
      :legend => [:align],
      :dl => [:compact],
      :input => [:align],
      :h6 => [:align],
      :hr => [:align, :noshade, :size, :width],
      :base => [:target],
      :ul => [:type, :compact],
      :br => [:clear],
      :form => [:name, :target],
      :area => [:target],
      :h1 => [:align]
    }.each do |k, v|
        @tagset[k] += v
    end

    @tags = @tagset.keys
    @forms = @tags & FORM_TAGS
    @self_closing = @tags & SELF_CLOSING_TAGS
  end

  # The Markaby::Builder class is the central gear in the system.  When using
  # from Ruby code, this is the only class you need to instantiate directly.
  #
  #   mab = Markaby::Builder.new
  #   mab.html do
  #     head { title "Boats.com" }
  #     body do
  #       h1 "Boats.com has great deals"
  #       ul do
  #         li "$49 for a canoe"
  #         li "$39 for a raft"
  #         li "$29 for a huge boot that floats and can fit 5 people"
  #       end
  #     end
  #   end
  #   puts mab.to_s
  #
  class Builder

    @@default = {
      :indent => 0,
      :output_helpers => true,
      :output_xml_instruction => true,
      :output_meta_tag => true,
      :auto_validation => true,
      :tagset => Markaby::XHTMLTransitional
    }

    def self.set(option, value)
      @@default[option] = value
    end

    def self.ignored_helpers 
      @@ignored_helpers ||= [] 
    end 
 
    def self.ignore_helpers(*helpers) 
      ignored_helpers.concat helpers 
    end 

    attr_accessor :output_helpers, :tagset

    # Create a Markaby builder object.  Pass in a hash of variable assignments to
    # +assigns+ which will be available as instance variables inside tag construction
    # blocks.  If an object is passed in to +helpers+, its methods will be available
    # from those same blocks.
    #
    # Pass in a +block+ to new and the block will be evaluated.
    #
    #   mab = Markaby::Builder.new {
    #     html do
    #       body do
    #         h1 "Matching Mole"
    #       end
    #     end
    #   }
    #
    def initialize(assigns = {}, helpers = nil, &block)
      @streams = [[]]
      @assigns = assigns
      @elements = {}

      @@default.each do |k, v|
        instance_variable_set("@#{k}", @assigns[k] || v)
      end

      if helpers.nil?
        @helpers = nil
      else
        @helpers = helpers.dup
        for iv in helpers.instance_variables
          instance_variable_set(iv, helpers.instance_variable_get(iv))
        end
      end

      unless assigns.nil? || assigns.empty?
        for iv, val in assigns
          instance_variable_set("@#{iv}", val)
          unless @helpers.nil?
            @helpers.instance_variable_set("@#{iv}", val)
          end
        end
      end

      @builder = ::Builder::XmlMarkup.new(:indent => @indent, :target => @streams.last)
      class << @builder
          attr_accessor :target, :level
      end

      if block
        text(capture(&block))
      end
    end

    # Returns a string containing the HTML stream.  Internally, the stream is stored as an Array.
    def to_s
      @streams.last.to_s
    end

    # Write a +string+ to the HTML stream without escaping it.
    def text(string)
      @builder << "#{string}"
      nil
    end
    alias_method :<<, :text
    alias_method :concat, :text

    # Emulate ERB to satisfy helpers like <tt>form_for</tt>.
    def _erbout; self end

    # Captures the HTML code built inside the +block+.  This is done by creating a new
    # stream for the builder object, running the block and passing back its stream as a string.
    #
    #   >> Markaby::Builder.new.capture { h1 "TEST"; h2 "CAPTURE ME" }
    #   => "<h1>TITLE</h1>\n<h2>CAPTURE ME</h2>\n"
    #
    def capture(&block)
      @streams.push(builder.target = [])
      @builder.level += 1
      str = instance_eval(&block)
      str = @streams.last.join if @streams.last.any?
      @streams.pop
      @builder.level -= 1
      builder.target = @streams.last
      str
    end

    # Create a tag named +tag+. Other than the first argument which is the tag name,
    # the arguments are the same as the tags implemented via method_missing.
    def tag!(tag, *args, &block)
      ele_id = nil
      if @auto_validation and @tagset
          if !@tagset.tagset.has_key?(tag)
              raise InvalidXhtmlError, "no element `#{tag}' for #{tagset.doctype}"
          elsif args.last.respond_to?(:to_hash)
              attrs = args.last.to_hash
              attrs.each do |k, v|
                  atname = k.to_s.downcase.intern
                  unless k =~ /:/ or @tagset.tagset[tag].include? atname
                      raise InvalidXhtmlError, "no attribute `#{k}' on #{tag} elements"
                  end
                  if atname == :id
                      ele_id = v.to_s
                      if @elements.has_key? ele_id
                          raise InvalidXhtmlError, "id `#{ele_id}' already used (id's must be unique)."
                      end
                  end
              end
          end
      end
      if block
        str = capture &block
        block = proc { text(str) }
      end

      f = fragment { @builder.method_missing(tag, *args, &block) }
      @elements[ele_id] = f if ele_id
      f
    end

    # This method is used to intercept calls to helper methods and instance
    # variables.  Here is the order of interception:
    #
    # * If +sym+ is a helper method, the helper method is called
    #   and output to the stream.
    # * If +sym+ is a Builder::XmlMarkup method, it is passed on to the builder object.
    # * If +sym+ is also the name of an instance variable, the
    #   value of the instance variable is returned.
    # * If +sym+ has come this far and no +tagset+ is found, +sym+ and its arguments are passed to tag!
    # * If a tagset is found, though, +NoMethodError+ is raised.
    #
    # method_missing used to be the lynchpin in Markaby, but it's no longer used to handle
    # HTML tags.  See html_tag for that.
    def method_missing(sym, *args, &block)
      if @helpers.respond_to?(sym, true) && !self.class.ignored_helpers.include?(sym)
        r = @helpers.send(sym, *args, &block)
        if @output_helpers and r.respond_to? :to_str
          fragment { @builder << r }
        else
          r
        end
      elsif ::Builder::XmlMarkup.instance_methods.include?(sym.to_s) 
        @builder.__send__(sym, *args, &block)
      elsif instance_variables.include?("@#{sym}")
        instance_variable_get("@#{sym}")
      elsif @tagset.nil?
        tag!(sym, *args, &block)
      else
        raise NoMethodError, "no such method `#{sym}'"
      end
    end

    # Every HTML tag method goes through an html_tag call.  So, calling <tt>div</tt> is equivalent
    # to calling <tt>html_tag(:div)</tt>.  All HTML tags in Markaby's list are given generated wrappers
    # for this method.
    #
    # If the @auto_validation setting is on, this method will check for many common mistakes which
    # could lead to invalid XHTML.
    def html_tag(sym, *args, &block)
      if @auto_validation and @tagset.self_closing.include?(sym) and block
        raise InvalidXhtmlError, "the `\#{sym}' element is self-closing, please remove the block"
      end
      if args.empty? and block.nil? and not NO_PROXY.include?(sym)
        return CssProxy.new do |args, block|
          if @tagset.forms.include?(sym) and args.last.respond_to?(:to_hash) and args.last[:id]
            args.last[:name] ||= args.last[:id]
          end
          tag!(sym, *args, &block)
        end
      end
      if not @tagset.self_closing.include?(sym) and args.first.respond_to?(:to_hash)
        block ||= proc{}
      end
      tag!(sym, *args, &block)
    end

    XHTMLTransitional.tags.each do |k|
      class_eval %{
        def #{k}(*args, &block)
          html_tag(#{k.inspect}, *args, &block)
        end
      }
    end

    # Builds a head tag.  Adds a <tt>meta</tt> tag inside with Content-Type
    # set to <tt>text/html; charset=utf-8</tt>.
    def head(*args, &block)
      tag!(:head, *args) do
        tag!(:meta, "http-equiv" => "Content-Type", "content" => "text/html; charset=utf-8") if @output_meta_tag
        instance_eval(&block)
      end
    end

    # Builds an html tag.  An XML 1.0 instruction and an XHTML 1.0 Transitional doctype
    # are prepended.  Also assumes <tt>:xmlns => "http://www.w3.org/1999/xhtml",
    # :lang => "en"</tt>.
    def xhtml_transitional(&block)
      self.tagset = Markaby::XHTMLTransitional
      xhtml_html &block
    end

    # Builds an html tag with XHTML 1.0 Strict doctype instead.
    def xhtml_strict(&block)
      self.tagset = Markaby::XHTMLStrict
      xhtml_html &block
    end

    private

    def xhtml_html(&block)
      instruct! if @output_xml_instruction
      declare!(:DOCTYPE, :html, :PUBLIC, *tagset.doctype)
      tag!(:html, :xmlns => "http://www.w3.org/1999/xhtml", "xml:lang" => "en", :lang => "en", &block)
    end

    def fragment
      stream = @streams.last
      f1 = stream.length
      yield
      f2 = stream.length - f1
      Fragment.new(stream, f1, f2)
    end

  end

  # Every tag method in Markaby returns a Fragment.  If any method gets called on the Fragment,
  # the tag is removed from the Markaby stream and given back as a string.  Usually the fragment
  # is never used, though, and the stream stays intact.
  #
  # For a more practical explanation, check out the README.
  class Fragment < ::Builder::BlankSlate
    def initialize(s, a, b)
      @s, @f1, @f2 = s, a, b 
    end
    def method_missing(*a)
      unless @str
        @str = @s[@f1, @f2].to_s  
        @s[@f1, @f2] = [nil] * @f2
        @str
      end
      @str.send(*a)
    end
  end

end
