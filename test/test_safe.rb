#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class TestSafe < Test::Unit::TestCase
if RUBY_VERSION >= "1.8.5"
  def setup
    @s = Sandbox.safe
  end

  def eval str
    @s.eval str
  end

  def test_globals
    assert_equal "(sandbox)", eval("$0")
  end
end

  def test_ruby_version
    assert_equal RUBY_VERSION >= "1.8.5", Sandbox.respond_to?(:safe)
  end
end
