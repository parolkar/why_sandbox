#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class TestRefsEmptyClass
  def a_method
    :a_method
  end
end

module TestRefsModule
  def self.query(str)
    "QUERIED: #{str}"
  end
end

class TestRefs < Test::Unit::TestCase
  def setup
    @s = Sandbox.safe
  end

  def test_empty_class
    @s.ref TestRefsEmptyClass
    assert_equal "TestRefsEmptyClass", @s.eval("TestRefsEmptyClass.name")
    assert_equal "BoxedClass", @s.eval("TestRefsEmptyClass.superclass.name")
    assert_not_equal TestRefsEmptyClass.object_id, @s.eval("TestRefsEmptyClass.object_id")
    assert_equal @s.eval("TestRefsEmptyClass.object_id"), @s.eval("TestRefsEmptyClass.new.class.object_id")
    assert_equal :a_method, @s.eval("TestRefsEmptyClass.new.a_method")
  end

  def test_module
    @s.ref TestRefsModule
    assert_equal "QUERIED: SELECT * FROM freakyfreaky",
                 @s.eval("TestRefsModule.query('SELECT * FROM freakyfreaky')")
  end

  def test_recursive_sandbox
    @s.import Sandbox::Safe
    assert_equal @s.eval(%{s = Sandbox.new; s.eval "2+2"}), 4
  end
end
