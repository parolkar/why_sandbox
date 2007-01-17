require 'sand_table'
require 'thread'

module Sandbox

  BUILD = "#{VERSION}.#{REV_ID[6..-3]}" #:nodoc:
  PRELUDE = File.expand_path("../sandbox/prelude.rb", __FILE__) #:nodoc:

  #
  # Stands in for an uncaught exception raised within the sandbox during evaluation.
  # (See Sandbox#eval.)
  #
  class Exception
  end

  #
  # Raised when the duration of a sandbox evaluation exceeds a specified
  # timeout.  (See Sandbox#eval.)
  #
  class TimeoutError < Exception
  end

  class Full
    private :_eval
    #
    # call-seq:
    #    sandbox.eval(str, opts={})   => obj
    #
    # Evaluates +str+ as Ruby code inside the sandbox and returns
    # the result.  If an option hash +opts+ is provided, any options
    # specified in it take precedence over options specified when +sandbox+
    # was created.  (See Sandbox.new.)
    #
    # Available options include:
    #
    # [:timeout]  The maximum time in seconds which Sandbox#eval is allowed to
    #             run before it is forcibly terminated.
    # [:safelevel]  The $SAFE level to use during evaluation in the sandbox.
    #
    # If evaluation times out, Sandbox#eval will raise a
    # Sandbox::TimeoutError.  If no timeout is specified, Sandbox#eval will
    # be allowed to run indefinitely.
    #
    def eval(str, opts = {})
      opts = @options.merge(opts)
      if opts[:timeout] or opts[:safelevel]
        th, exc, timed_out = nil, nil, false
        safelevel = opts[:safelevel]
        th = Thread.start(str) do
          $SAFE = safelevel if safelevel and safelevel > $SAFE
          begin
            _eval(str)
          rescue Exception => exc
          end
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
        if timed_out
          raise TimeoutError, "#{self.class}#eval timed out"
        elsif exc
          raise exc
        else
          th.value
        end
      else
        _eval(str)
      end
    end

    #
    # call-seq:
    #    sandbox.load(portname, opts={})   => obj
    #
    # Reads all available data from the given I/O port +portname+ and
    # then evaluates it as a string in +sandbox+.  (See Sandbox#eval.)
    #
    def load(io, opts = {})
      eval(IO.read(io), opts)
    end
  end

end
