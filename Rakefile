require 'rake'
require 'rake/clean'
require 'rake/gempackagetask'
require 'rake/rdoctask'
require 'rake/testtask'
require 'fileutils'
include FileUtils

NAME = "sandbox"
REV = File.read(".svn/entries")[/committed-rev="(\d+)"/, 1] rescue nil
VERS = ENV['VERSION'] || ("0.3" + (REV ? ".#{REV}" : ""))
FILES = %w(COPYING README Rakefile setup.rb {bin,doc,test,extras}/**/* lib/**/*.rb ext/**/extconf.rb ext/**/*.{h,c})
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
Rake::PackageTask.new(NAME, VERS) do |p|
  p.need_tar = true
  p.package_files.include FILES
end
### Add 'sand_table_win32' to task deps
# Rake::PackageTask.new(NAME, VERS + "-mswin32") do |p|
#   p.need_tar = true
#   p.package_files.include FILES
#   p.package_files.include %w(lib/i386-msvcrt/**/*)
# end

desc "Run all the tests"
Rake::TestTask.new do |t|
    t.libs << "test"
    t.test_files = FileList['test/test_*.rb']
    t.verbose = true
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

desc "Cross-compile the sand_table extension for win32"
file "sand_table_win32" do
  cp "extras/mingw-rbconfig.rb", "ext/sand_table/rbconfig.rb"
  sh "cd ext/sand_table/ && ruby -I. extconf.rb && make"
  mkdir "lib/i386-msvcrt"
  mv "ext/sand_table/sand_table.so", "lib/i386-msvcrt/"
  rm "ext/sand_table/rbconfig.rb"
end

task :install do
  sh %{rake package}
  sh %{sudo gem install pkg/#{NAME}-#{VERS}}
end

task :uninstall => [:clean] do
  sh %{sudo gem uninstall #{NAME}}
end
