#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class TestRefsEmptyClass
  def a_method
  end
end

class TestRefs < Test::Unit::TestCase
  def setup
    @s = Sandbox.safe
  end

  def test_empty_class
    @s.ref TestRefsEmptyClass
    assert_equal "TestRefsEmptyClass", @s.eval("TestRefsEmptyClass.name")
    assert_not_equal TestRefsEmptyClass.object_id, @s.eval("TestRefsEmptyClass.object_id")
    assert_equal @s.eval("TestRefsEmptyClass.object_id"), @s.eval("TestRefsEmptyClass.new.class.object_id")
    assert_equal :a_method, @s.eval("TestRefsEmptyClass.new.a_method")
  end
end
