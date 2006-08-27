#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class TestImport < Test::Unit::TestCase
  def test_pwd
    box = Sandbox.new :import => [IO, Dir, File]
    assert_equal Dir.pwd, box.eval("Dir.pwd")
  end

  def test_caller
    box = Sandbox.new :init => [:all]
    assert_equal true, box.eval("Kernel.respond_to? :caller")
  end
end
