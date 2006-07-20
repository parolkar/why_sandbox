#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class TestPlain < Test::Unit::TestCase
  def path(p)
    File.join(File.dirname(__FILE__), p)
  end

  def setup
    @s = Sandbox.new
  end

  def eval str
    @s.eval str
  end

  def test_monkeypatch_the_return
    $TEST_PLAIN_UNSAFE_PROC = proc { open(path("files/pascal.rb")) }
    class << eval("Kernel")
      def crack_yes_this_is_allowed
        $TEST_PLAIN_UNSAFE_PROC.call.read.length
      end
    end
    assert ! Kernel.respond_to?(:crack_yes_this_is_allowed)
    assert eval("Kernel.respond_to?(:crack_yes_this_is_allowed)")
    assert_equal 449, eval("Kernel.crack_yes_this_is_allowed")
  end
end
