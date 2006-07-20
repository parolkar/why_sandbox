#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class TestPlain < Test::Unit::TestCase
  def setup
    @s = Sandbox.new
  end

  def eval str
    @s.eval str
  end

  def test_monkeypatch_the_return
    k = eval("Kernel")
    def k.crack_yes_this_is_allowed
      open("/etc/passwd")
    end
    assert ! Kernel.respond_to?(:crack_yes_this_is_allowed)
    assert eval("Kernel.respond_to?(:crack_yes_this_is_allowed)")
  end
end
