require 'sand_table'
require 'thread'

class Sandbox

  class TimeoutError < Exception
  end

  BUILD = "#{VERSION}.#{REV_ID[6..-3]}"

  def eval(str, opts = {})
    opts = @options.merge(opts)
    if opts[:timeout]
      timed_out = false
      begin
        th = Thread.start(str) { |code| $SAFE = 4; _eval(code) }
        th.join(opts[:timeout])
        if th.alive?
          if th.respond_to? :kill!
            th.kill!
          else
            th.kill
          end
          timed_out = true
        end
        th.value
      ensure
        finish
      end
      raise TimeoutError, "Sandbox#eval timed out" if timed_out
    else
      _eval(str)
    end
  end

  def load(io, opts = {})
    eval(IO.read(io), opts)
  end

end
