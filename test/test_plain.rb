#!/usr/bin/env ruby

require 'sandbox'
require 'test/unit'

class Book; end

class EmptyClass
  def empty_method
  end
end

module OneNested
  class FirstClass
  end
end

module NestA
  module NestB
    class FirstClass
    end
    class SecondClass
    end
  end
end

module NestC
  class FirstClass < NestA::NestB::FirstClass
  end
  class SecondClass < NestA::NestB::SecondClass
  end
end

class WeirdInherit < NestA::NestB::SecondClass
  module A; end
end

class TestPlain < Test::Unit::TestCase
  def path(p)
    File.join(File.dirname(__FILE__), p)
  end

  def setup
    @s = Sandbox.new
    @s.eval %{
      class Book
        attr_accessor :author, :title
        def initialize(a, t)
          @author, @title = a, t
        end
      end
    }
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

  def test_class_in_sandbox
    book = eval("B = Book.new('Stanislaw Lem', 'The Star Diaries')")
    assert_equal "Book", book.class.name
    assert book.class != Book
    assert book.class == eval("Book")

    eval("Book").class_eval do
      def year
        1970
      end
    end
    assert eval("B.year == 1970")
  end

  def test_obj_passing
    a = 1
    book = eval("b = Book.new('Stanislaw Lem', 'The Star Diaries')")
    assert_equal book.author, 'Stanislaw Lem'
    assert_equal book.title, 'The Star Diaries'
    assert_equal 1, eval("local_variables").length
  end

  def test_import_paths
    @s.import EmptyClass
    assert_equal EmptyClass.name, eval("EmptyClass.name")
    # assert eval("EmptyClass.respond_to?(:empty_method)")
    @s.import OneNested::FirstClass
    assert_equal OneNested.name, eval("OneNested.name")
    assert_equal Module.name, eval("OneNested.class.name")
    assert_equal OneNested::FirstClass.name, eval("OneNested::FirstClass.name")
    @s.import NestC::FirstClass
    assert_equal NestC::FirstClass.name, eval("NestC::FirstClass.name")
    assert_equal NestA::NestB::FirstClass.name, eval("NestA::NestB::FirstClass.name")
    assert_equal NestA::NestB.class.name, eval("NestA::NestB.class.name")
    @s.import NestC::SecondClass
    assert_equal NestC::SecondClass.name, eval("NestC::SecondClass.name")
    assert_equal NestC::SecondClass.class.name, eval("NestC::SecondClass.class.name")
    assert_equal NestA::NestB::SecondClass.name, eval("NestA::NestB::SecondClass.name")
    assert_equal NestA::NestB.class.name, eval("NestA::NestB.class.name")
    @s.import NestA::NestB::SecondClass
    @s.import NestC::SecondClass
    assert_equal NestC::SecondClass.name, eval("NestC::SecondClass.name")
    assert_equal NestC::SecondClass.class.name, eval("NestC::SecondClass.class.name")
    assert_equal NestA::NestB::SecondClass.name, eval("NestA::NestB::SecondClass.name")
    assert_equal NestA::NestB.class.name, eval("NestA::NestB.class.name")
  end

  def test_import_conflict
    eval("class WeirdInherit < Class.new; end")
    assert_raise TypeError do
      @s.import WeirdInherit
    end
    assert_raise TypeError do
      @s.import WeirdInherit::A
    end
  end

  def test_import_methods
  end
end
