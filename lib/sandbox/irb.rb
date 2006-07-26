require 'irb'
require 'sandbox'

class Sandbox::IRB

  def initialize(box)
    @box, @sig, @p = box, :IN_IRB
    @prompt = {:start => ">> ", :continue => ".. ", :nested => ".. ",
      :string => "   ", :return => "=> %s\n"}
  end

  def box_eval(str)
    @box.eval(str)
  end

  def start(io)
    scanner = RubyLex.new
    scanner.exception_on_syntax_error = false
    scanner.set_prompt do |ltype, indent, continue, line_no|
      if ltype
        f = @prompt[:string]
      elsif continue
        f = @prompt[:continue]
      elsif indent > 0
        f = @prompt[:nested]
      else
        f = @prompt[:start]
      end
      f = "" unless f
      @p = prompt(f, ltype, indent, line_no)
    end

    scanner.set_input(io) do
      signal_status(:IN_INPUT) do
        io.print @p
        io.gets
      end
    end

    scanner.each_top_level_statement do |line, line_no|
      signal_status(:IN_EVAL) do
        line.untaint
        val = box_eval(line)
        io.puts @prompt[:return] % [val.inspect]
      end
    end
  end

  def prompt(prompt, ltype, indent, line_no)
    p = prompt.dup
    p.gsub!(/%([0-9]+)?([a-zA-Z])/) do
      case $2
      # when "N"
      #   @context.irb_name
      # when "m"
      #   @context.main.to_s
      # when "M"
      #   @context.main.inspect
      when "l"
        ltype
      when "i"
        if $1 
          format("%" + $1 + "d", indent)
        else
          indent.to_s
        end
      when "n"
        if $1 
          format("%" + $1 + "d", line_no)
        else
          line_no.to_s
        end
      when "%"
        "%"
      end
    end
    p
  end

  def signal_status(status)
    return yield if @sig == :IN_LOAD
    sig_back = @sig
    @sig = status
    begin
      yield
    ensure
      @sig = sig_back
    end
  end
end
