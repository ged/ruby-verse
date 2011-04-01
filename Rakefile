#!/usr/bin/env rake

require 'pathname'
require 'hoe'
require 'rake/extensiontask'
require 'rake/clean'
require 'erb'

BASEDIR = Pathname( __FILE__ ).dirname
LIBDIR = BASEDIR + 'lib'
EXTDIR = BASEDIR + 'ext'

NODE_TYPES = %w[audio bitmap curve geometry material object text]
NODE_TYPE_REGEX = %r{#{EXTDIR}/(#{Regexp.union(NODE_TYPES)})node\.c}
NODE_TYPE_SOURCE_PATHS = NODE_TYPES.
	collect {|nodetype| EXTDIR + "#{nodetype}node.c" }.
	collect {|path| path.to_s }
NODE_TYPE_SOURCES = Rake::FileList[ *NODE_TYPE_SOURCE_PATHS ]

NODE_TYPE_TEMPLATE = EXTDIR + 'nodeclass.template'


# Hoe plugins
Hoe.plugin :mercurial
Hoe.plugin :signing
Hoe.plugin :highline

Hoe.plugins.delete :rubyforge

hoespec = Hoe.spec 'verse' do
	self.readme_file = 'README.md'
	self.history_file = 'History.md'

	self.developer 'Michael Granger', 'ged@FaerieMUD.org'

	self.extra_dev_deps.push *{
		'rspec' => '~> 2.5',
	}

	self.spec_extras[:licenses] = ["BSD"]
	self.require_ruby_version( '>=1.8.7' )
	self.rdoc_locations << "deveiate:/usr/local/www/public/code/#{remote_rdoc_dir}"
end

ENV['VERSION'] ||= hoespec.spec.version.to_s

desc "Clobber the existing source for the node classes and replace them with blank templated source."
task :remake_node_classes do
	say "This will REMOVE all the child node classes and replace it with blank templated source."
	if agree( "Are you sure you want to do this? " )
		FileUtils.rm_f( NODE_TYPE_SOURCES, :verbose => true )
	end
	Rake::Task[ :node_classes ].invoke
end

file *NODE_TYPE_SOURCES
task :node_classes => NODE_TYPE_SOURCES
task :compile => NODE_TYPE_SOURCES

### Generate a skeleton at +target+ for the specified +nodetype+ using the NODE_TYPE_TEMPLATE.
def gen_nodetype_source( nodetype, target )
	trace "nodetype is %p" % [ nodetype ]

	template = ERB.new( NODE_TYPE_TEMPLATE.read, nil, '<>' )
	sourcecode = template.result( binding() )

	File.open( target, File::EXCL|File::CREAT|File::WRONLY, 0644 ) do |ofh|
		ofh.print( sourcecode )
	end
end

rule NODE_TYPE_REGEX => NODE_TYPE_TEMPLATE.to_s do |task|
	log "  creating %s from %s..." % [ task.name, NODE_TYPE_TEMPLATE ]
	task.name =~ NODE_TYPE_REGEX
	nodetype = $1 or abort "Ack! No nodetype?!?"
	gen_nodetype_source( nodetype, task.name )
end


Rake::ExtensionTask.new do |ext|
    ext.name = 'verse_ext'
    ext.gem_spec = hoespec.spec
    ext.ext_dir = EXTDIR.to_s
	ext.source_pattern = "*.{c,h}"
	ext.cross_compile = true
	ext.cross_platform = %w[i386-mswin32 i386-mingw32]
end


