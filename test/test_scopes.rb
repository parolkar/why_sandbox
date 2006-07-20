#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class TestScopes < Test::Unit::TestCase
  def setup
    @src = "a = {}; 50.times { |i| a[i] = i }; a"
  end

  def test_same_scope
    s = Sandbox.new
    s.eval(@src)
    assert_equal 1, s.eval("a[1]")
  end

  def test_block_scope
    50.times do
      s = Sandbox.new
      s.eval(@src)
      assert_equal 1, s.eval("a[1]")
    end
  end

  def test_block_vary
    s = Sandbox.new
    50.times do
      s.eval(@src)
      assert_equal 1, s.eval("a[1]")
    end
  end

  def test_as_ivar
    st = Struct.new(:sandbox)
    s = st.new
    s.sandbox = Sandbox.new
    50.times do
      s.sandbox.eval(@src)
      assert_equal 1, s.sandbox.eval("a[1]")
    end
  end

  def test_in_bench
    25.times do
      s = Sandbox.new
      2.times do
        s.eval(@src)
        assert_equal 1, s.eval("a[1]")
      end
    end
  end
end
