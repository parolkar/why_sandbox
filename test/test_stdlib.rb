#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class TestExploits < Test::Unit::TestCase
  def path(p)
    File.join(File.dirname(__FILE__), p)
  end
  def test_minilib
    s = Sandbox.new
    s.load(path("files/minilib.rb"))
    assert_raise NameError do
      MiniLib.add(1,2)
    end
    assert_equal 3, s.eval("MiniLib.add(1,2)")
    assert_equal 3, s.eval("::MiniLib.add(1,2)")
  end
  def test_pascal
    s = Sandbox.new
    s.load(path("files/pascal.rb"))
    pascal = s.eval("pascal 10").join.gsub(/\s+/, ' ').strip
    assert_equal pascal, 
      "1 1 1 1 2 1 1 3 3 1 1 4 6 4 1 1 5 10 10 5 1 1 6 15 20 15 6 1 1 7 21 35 35 21 7 1 1 8 28 56 70 56 28 8 1 1 9 36 84 126 126 84 36 9 1"
  end
  # def test_camping
  #   s = Sandbox.new
  #   s.load(path("files/camping.rb"))
  # end
end
