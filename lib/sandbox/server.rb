require 'socket'
require 'sandbox/irb'

class Sandbox::IRBServer
  attr_reader :acceptor
  attr_reader :workers
  attr_reader :host
  attr_reader :port
  attr_reader :timeout
  attr_reader :num_processors

  def initialize(host, port, num_processors=(2**30-1), timeout=0)
    @socket = TCPServer.new(host, port) 
    @host = host
    @port = port
    @workers = ThreadGroup.new
    @timeout = timeout
    @num_processors = num_processors
    @death_time = 60
    @sessions = {}
  end

  def randid
    abc = %{ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz} 
    (1..20).map { abc[rand(abc.size),1] }.join
  end

  def new_sandbox
    Sandbox.safe(:timeout => 10)
  end

  def process_client(client)
    begin
      case client.gets
      when /^LOGIN (\w+)/
        sess = $1
      else
        sess = randid
      end

      @sessions[sess] ||= new_sandbox
      client.puts sess
      Sandbox::IRB.new(@sessions[sess]).start(client)
    rescue EOFError,Errno::ECONNRESET,Errno::EPIPE,Errno::EINVAL,Errno::EBADF
      # ignored
    rescue Errno::EMFILE
      reap_dead_workers('too many files')
    rescue Object
      STDERR.puts "#{Time.now}: ERROR: #$!"
      STDERR.puts $!.backtrace.join("\n")
    ensure
      client.close unless client.closed?
    end
  end

  # Used internally to kill off any worker threads that have taken too long
  # to complete processing.  Only called if there are too many processors
  # currently servicing.  It returns the count of workers still active
  # after the reap is done.  It only runs if there are workers to reap.
  def reap_dead_workers(reason='unknown')
    if @workers.list.length > 0
      STDERR.puts "#{Time.now}: Reaping #{@workers.list.length} threads for slow workers because of '#{reason}'"
      mark = Time.now
      @workers.list.each do |w|
        w[:started_on] = Time.now if not w[:started_on]

        if mark - w[:started_on] > @death_time + @timeout
          STDERR.puts "Thread #{w.inspect} is too old, killing."
          w.raise(TimeoutError.new("Timed out thread."))
        end
      end
    end

    return @workers.list.length
  end

  # Performs a wait on all the currently running threads and kills any that take
  # too long.  Right now it just waits 60 seconds, but will expand this to
  # allow setting.  The @timeout setting does extend this waiting period by
  # that much longer.
  def graceful_shutdown
    while reap_dead_workers("shutdown") > 0
      STDERR.print "Waiting for #{@workers.list.length} requests to finish, could take #{@death_time + @timeout} seconds."
      sleep @death_time / 10
    end
  end


  # Runs the thing.  It returns the thread used so you can "join" it.  You can also
  # access the HttpServer::acceptor attribute to get the thread later.
  def run
    BasicSocket.do_not_reverse_lookup=true

    @acceptor = Thread.new do
      while true
        begin
          client = @socket.accept
          worker_list = @workers.list

          if worker_list.length >= @num_processors
            STDERR.puts "Server overloaded with #{worker_list.length} processors (#@num_processors max). Dropping connection."
            client.close
            reap_dead_workers("max processors")
          else
            thread = Thread.new { process_client(client) }
            thread.abort_on_exception = true
            thread[:started_on] = Time.now
            @workers.add(thread)

            sleep @timeout/100 if @timeout > 0
          end
        rescue StopServer
          @socket.close if not @socket.closed?
          break
        rescue Errno::EMFILE
          reap_dead_workers("too many open files")
          sleep 0.5
        rescue Errno::ECONNABORTED
          # client closed the socket even before accept
          client.close if not client.closed?
        end
      end

      graceful_shutdown
    end

    return @acceptor
  end

  # Stops the acceptor thread and then causes the worker threads to finish
  # off the request queue before finally exiting.
  def stop
    stopper = Thread.new do 
      exc = StopServer.new
      @acceptor.raise(exc)
    end
    stopper.priority = 10
  end

end
