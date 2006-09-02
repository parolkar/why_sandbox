require 'sand_table'
require 'thread'

class Sandbox

  BUILD = "#{VERSION}.#{REV_ID[6..-3]}"
  PRELUDE = File.expand_path("../sandbox/prelude.rb", __FILE__)

  class TimeoutError < Exception
  end

  def eval(str, opts = {})
    opts = @options.merge(opts)
    if opts[:timeout] or opts[:safelevel]
      th, timed_out = nil, false
      begin
        safelevel = opts[:safelevel]
        th = Thread.start(str) do
          $SAFE = safelevel if safelevel and safelevel > $SAFE
          _eval(str)
        end
        th.join(opts[:timeout])
        if th.alive?
          if th.respond_to? :kill!
            th.kill!
          else
            th.kill
          end
          timed_out = true
        end
      ensure
        finish
      end
      if timed_out
        raise TimeoutError, "Sandbox#eval timed out"
      else
        th.value
      end
    else
      _eval(str)
    end
  end

  def load(io, opts = {})
    eval(IO.read(io), opts)
  end

end
