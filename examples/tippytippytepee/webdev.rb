
module REST
  def self.invoke(url, params)
    Web.get(url + '&' + params.map { |k, v| k.to_s+'='+v.to_s }.join('&'))
  end
end 

module Wiki
 
  # find the <code> tag and include everything inside
  def self.get_code(uri)
    code = Hpricot(Web.get(uri)).at("code")
    return nil if code.nil?
    code.inner_html 
  end
  
  def self.get(wiki_node)
    Web.get(wiki_node)
  end
  def self.get_source(wiki_node)
    Jungle.get_source(wiki_node)
  end

  def self.import(wiki_node)
    src = Jungle.get_source(wiki_node.to_s)
    eval src
  end

end
