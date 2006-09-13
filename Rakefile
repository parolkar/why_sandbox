require 'rake'
require 'rake/clean'
require 'rake/gempackagetask'
require 'rake/rdoctask'
require 'rake/testtask'
require 'fileutils'
include FileUtils

NAME = "sandbox"
REV = File.read(".svn/entries")[/committed-rev="(\d+)"/, 1] rescue nil
VERS = ENV['VERSION'] || ("0.1" + (REV ? ".#{REV}" : ""))
CLEAN.include ['ext/sand_table/*.{bundle,o,so,obj,pdb,lib,def,exp}', 'ext/sand_table/Makefile', 
               '**/.*.sw?', '*.gem', '.config']

desc "Does a full compile, test run"
task :default => [:compile, :test]

desc "Compiles all extensions"
task :compile => [:sand_table] do
  if Dir.glob(File.join("lib","sand_table.*")).length == 0
    STDERR.puts "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    STDERR.puts "Gem actually failed to build.  Your system is"
    STDERR.puts "NOT configured properly to build Sandbox."
    STDERR.puts "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    exit(1)
  end
end

desc "Packages up Sandbox."
task :package => [:clean]

desc "Run all the tests"
Rake::TestTask.new do |t|
    t.libs << "test"
    t.test_files = FileList['test/test_*.rb']
    t.verbose = true
end

spec =
    Gem::Specification.new do |s|
        s.name = NAME
        s.version = VERS
        s.platform = Gem::Platform::RUBY
        s.has_rdoc = true
        s.rdoc_options.push '--exclude', '(test|extras)/.*', '--exclude', 'setup\.rb'
        s.extra_rdoc_files = ["README", "CHANGELOG", "COPYING"]
        s.summary = "a freaky-freaky sandbox library, copies the symbol table, mounts it, evals..."
        s.description = s.summary
        s.author = "why the lucky stiff"
        s.email = 'why@ruby-lang.org'
        s.homepage = 'http://code.whytheluckystiff.net/sandbox/'

        s.files = %w(COPYING README Rakefile setup.rb) +
          Dir.glob("{bin,doc,test,lib,extras}/**/*") + 
          Dir.glob("ext/**/extconf.rb") +
          Dir.glob("ext/**/*.{h,c}")
        
        s.required_ruby_version = '>= 1.8.5'
        s.require_path = "lib"
        s.extensions = FileList["ext/**/extconf.rb"].to_a
        s.bindir = "bin"
        s.executables = %w(sandbox_irb sandbox_server)
    end

Rake::GemPackageTask.new(spec) do |p|
    p.need_tar = true
    p.gem_spec = spec
end

extension = "sand_table"
ext = "ext/sand_table"
ext_so = "#{ext}/#{extension}.#{Config::CONFIG['DLEXT']}"
ext_files = FileList[
  "#{ext}/*.c",
  "#{ext}/*.h",
  "#{ext}/extconf.rb",
  "#{ext}/Makefile",
  "lib"
] 

task "lib" do
  directory "lib"
end

desc "Builds just the #{extension} extension"
task extension.to_sym => ["#{ext}/Makefile", ext_so ]

file "#{ext}/Makefile" => ["#{ext}/extconf.rb"] do
  Dir.chdir(ext) do ruby "extconf.rb" end
end

file ext_so => ext_files do
  Dir.chdir(ext) do
    sh(PLATFORM =~ /win32/ ? 'nmake' : 'make')
  end
  cp ext_so, "lib"
end

PKG_FILES = FileList[
  "test/**/*.{rb,html,xhtml}",
  "lib/**/*.rb",
  "bin/**/*",
  "ext/**/*.{c,rb,h,rl}",
  "CHANGELOG", "README", "Rakefile", "COPYING",
  "extras/**/*", "lib/sand_table.so"]

Win32Spec = Gem::Specification.new do |s|
  s.name = NAME
  s.version = VERS
  s.platform = Gem::Platform::WIN32
  s.has_rdoc = true
  s.rdoc_options.push '--exclude', '(test|extras)/.*', '--exclude', 'setup\.rb'
  s.extra_rdoc_files = ["README", "CHANGELOG", "COPYING"]
  s.summary = "a freaky-freaky sandbox library, copies the symbol table, mounts it, evals..."
  s.description = s.summary
  s.author = "why the lucky stiff"
  s.email = 'why@ruby-lang.org'
  s.homepage = 'http://code.whytheluckystiff.net/svn/sandbox/'

  s.files = PKG_FILES

  s.require_path = "lib"
  s.extensions = []
  s.bindir = "bin"
  s.executables = %w(sandbox_irb sandbox_server)
end
  
WIN32_PKG_DIR = "#{NAME}-#{VERS}"

file WIN32_PKG_DIR => [:package] do
  sh "tar zxf pkg/#{WIN32_PKG_DIR}.tgz"
end

desc "Cross-compile the sand_table extension for win32"
file "sand_table_win32" => [WIN32_PKG_DIR] do
  cp "extras/mingw-rbconfig.rb", "#{WIN32_PKG_DIR}/ext/sand_table/rbconfig.rb"
  sh "cd #{WIN32_PKG_DIR}/ext/sand_table/ && ruby -I. extconf.rb && make"
  mv "#{WIN32_PKG_DIR}/ext/sand_table/sand_table.so", "#{WIN32_PKG_DIR}/lib"
end

desc "Build the binary RubyGems package for win32"
task :rubygems_win32 => ["sand_table_win32"] do
  Dir.chdir("#{WIN32_PKG_DIR}") do
    Gem::Builder.new(Win32Spec).build
    verbose(true) {
      mv Dir["*.gem"].first, "../pkg/#{WIN32_PKG_DIR}-mswin32.gem"
    }
  end
end

CLEAN.include WIN32_PKG_DIR

task :install do
  sh %{rake package}
  sh %{sudo gem install pkg/#{NAME}-#{VERS}}
end

task :uninstall => [:clean] do
  sh %{sudo gem uninstall #{NAME}}
end
