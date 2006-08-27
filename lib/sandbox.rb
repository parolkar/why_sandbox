require 'sand_table'
require 'thread'

class Sandbox

  BUILD = "#{VERSION}.#{REV_ID[6..-3]}"

  def eval(str, opts = {})
    opts = @options.merge(opts)
    if opts[:timeout]
      begin
        th = Thread.start(str) { |code| $SAFE = 4; _eval(code) }
        th.join(opts[:timeout])
        th.raise _eval("Exception") if th.alive?
      ensure
        finish
      end
    else
      _eval(str)
    end
  end

  def load(io, opts = {})
    eval(IO.read(io), opts)
  end

end
