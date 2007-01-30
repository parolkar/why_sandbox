
module REST
  def self.invoke(url, params)
    Web.get(url + '&' + params.map { |k, v| k.to_s+'='+v.to_s }.join('&'))
  end
end 

module Wiki
  def self.get_code(uri)
    code = Hpricot(Web.get(uri)).at("code")
    return nil if code.nil?
    code.inner_html 
  end

  def self.import(wiki_node)
    src = self.get_code(wiki_node.to_s)
    eval src
  end
end
