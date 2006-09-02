#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class TestTimeout < Test::Unit::TestCase

  def setup
    @s = Sandbox.safe(:timeout => 0.5)
  end

  def eval str
    @s.eval str
  end

  def test_endless
    assert_raise Sandbox::TimeoutError do
      eval %{
        endless = proc do
          begin
            loop {}
          ensure
            endless[]
          end
        end

        endless[]
      }
    end
  end

  def test_math_stalls
    assert_raise Sandbox::TimeoutError do
      eval %{99999999**999999999}
    end
  end

end
