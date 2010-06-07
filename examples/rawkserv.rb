#!/usr/bin/env ruby

#
# Rawk server, a Ruby port of the Tawk basic IRC-like chat for 
# Verse. *** UNFINISHED ***
#
VERSION = "0.1"

require 'verse'

the_avatar  = None
the_methods = None
the_channels = {}
the_users    = {}

class VerseMethod

	def initialize( session, name, params )
		@session = session
		@name    = name
		@params  = params
	end

	def call( node_id, group_id, args )
		@session.call_method( node_id, group_id, args )
	end

end # class VerseMethod


class VerseMethodGroup

	def initialize( id, session )
		@id             = id
		@session        = session
		@remote_methods = {}
	end

	def add( name, params )
		self.remote_methods[ name ] = VerseMethod.new( @session, name, params )
	end

	def has?( *names )
		return names.all? {|name| self.remote_methods.key?(name) }
	end

	def get( id )
		return self.remote_methods.find {|m| m.id == id }
	end

	def call( name, node_id, args )
		if meth = self.remote_methods[ name ]
			
		else
			raise NoMethodError, "no such method '#{name}' in this group" unless
				self.has?( name )
		end
	end
end # class VerseMethodGroup



# class User
# 
# 	def initialize( session, name )
# 		@session    = session
# 		@name       = name
# 		@channels   = []
# 		@rpcmethods = []
# 		@pending    = true
# 	end
# 
# 
# 	######
# 	public
# 	######
# 
# 	attr_reader :session, :name, :channels
# 	attr_accessor :methods, :pending
# 
# 
# 	def to_s
# 		return self.name
# 	end
# 
# 	def unpend(self):
# 		self.pending = false
# 		puts " user #{self.name} no longer pending"
# 
# 		self.session.unsubscribe_method_group( self.methods.id )
# 		self.send_hear( "#server", "server",
# 			"Welcome to rawk, the Verse simple chat system, " + self.name )
# 	end
# 
# 	def add_method( method_id, name, params )
# 		self.methods.add( method_id, name, params )
# 		if self.pending && 
# 			self.methods.has("join") and self.methods.has("leave") and self.methods.has("hear"):
# 				self.unpend()
# 
# 	def join( chname):
# 		global the_channels
# 
# 		try:	ch = the_channels[chname]
# 		except:	ch = None
# 		if ch == None:		# Joining a non-existant channel creates it.
# 			ch = Channel(chname)
# 			the_channels[chname] = ch
# 		if not ch in self.channels:
# 			self.channels.append(ch)
# 			ch.join(self)
# 			self.send_join(chname)
# 
# 	def leave( channel):
# 		if channel in self.channels:
# 			self.channels.remove(channel)
# 
# 	def send_hear( channel, sender, text):
# 		print "sending text to", self.name + ", " + str(self.id)
# 		self.methods.call("hear", self.id, (channel, sender, text))
# 
# 	def send_join( channel):
# 		print "sending join to", self.name
# 		self.methods.call("join", self.id, (channel, ))
# 
# 	def logged_out(self):
# 		print "logging out", self.name, "from all channels"
# 		for c in self.channels:
# 			channel_leave(c, self)
# 
# class Channel:
# 	def initialize( name):
# 		self.name = name
# 		self.members = []	# User references
# 		print "channel", name, "created"
# 
# 	def join( user):
# 		if not user in self.members:
# 			self.members += [user]
# 			print "added", user, "to channel", self.name
# 			user.send_hear(self.name, "server", "users in " +self.name + ":" + str([x.name for x in self.members]))
# 			for m in self.members:
# 				if m != user:
# 					m.send_hear(self.name, "server", "--> " + user.name + " has joined " + self.name)	
# 
# 	def leave( user):
# 		if not user in self.members:
# 			return
# 		self.members.remove(user)
# 		print "removed", user, "from channel", self.name
# 		for m in self.members:
# 			m.send_hear(self.name, "server", "<-- " + user.name + " has left " + self.name)
# 
# 	def say( channel, user, text):
# 		if user in self.members:
# 			for m in self.members:
# 				m.send_hear(channel, user.name, text)
# 
# def channel_leave(channel, user):
# 	global the_channels
# 	user.leave(channel)
# 	channel.leave(user)
# 	if len(channel.members) == 0:
# 		print "channel", channel.name, "has no users, removing it"
# 		del(the_channels[channel.name])
# 
# class Method:
# 	def initialize( id, name, params):
# 		self.id = id
# 		self.name = name
# 		self.params = params
# 
# class Methods:
# 	def initialize( id):
# 		self.id     = id
# 		self.info   = None
# 		self.login  = None
# 		self.logout = None
# 		self.join   = None
# 		self.leave  = None
# 		self.say    = None
# 
# def do_info(sender, params):
# 	global the_users, the_channels
# 	try:
# 		user = the_users[sender]
# 		what = params[0][1]
# 	except:	return
# 
# 	if what == "channels":
# 		user.send_hear("#server", "server", "channels:" + str([x.name+"("+str(len(x.members))+")" for x in the_channels.values()]))
# 	elif what == "users":
# 		user.send_hear("#server", "server", "users:" + str([x.name+"("+str(len(x.channels))+")" for x in the_users.values()]))
# 	elif what == "help":
# 		user.send_hear("#server", "server", "commands: info, login, logout, join, leave, say")
# 
# def do_login(sender, params):
# 	global the_users
# 	try:	a0 = params[0]
# 	except:	return
# 	print sender, "wants to log in as", a0[1]
# 	if not sender in the_users.keys():
# 		u = User(sender, a0[1])
# 		the_users[sender] = u
# 		v.send_node_subscribe(sender)
# 		print " fine"
# 	else:
# 		print "But there can be only one"
# 
# def do_logout(sender, params):
# 	global the_users
# 	if sender in the_users.keys():
# 		del the_users[sender]
# 
# def do_join(sender, params):
# 	try:	a0 = params[0]
# 	except:	return
# 	if a0[0] == v.O_METHOD_PTYPE_STRING:
# 		global the_users
# 		chn = a0[1]
# 		try:	u = the_users[sender]
# 		except:	u = None
# 		if u != None:
# 			u.join(chn)
# 		else:
# 			print "can't join channel without logging in first--user", sender, "unknown"
# 
# def do_leave(sender, params):
# 	global the_channels, the_users
# 	print "processing leave", params
# 	try:
# 		chn  = params[0][1]
# 		ch   = the_channels[chn]
# 		user = the_users[sender]
# 	except:	return
# 	channel_leave(ch, user)
# 
# def do_say(sender, params):
# 	global the_channels, the_users
# 	try:
# 		chn = params[0][1]
# 		msg = params[1][1]
# 		ch  = the_channels[chn]
# 		user = the_users[sender]
# 	except:	return
# 	ch.say(chn, user, msg)
# 
# def cb_node_destroy(node_id):
# 	global the_users
# 	try:	user = the_users[node_id]
# 	except:	return
# 	user.logged_out()
# 	del the_users[user.id]
# 
# def cb_o_method_group_create(node, group_id, name):
# 	global the_avatar, the_methods, the_users
# 	if node == the_avatar:
# 		the_methods = Methods(group_id)
# 		v.send_o_method_group_subscribe(node, group_id)
# 		v.send_o_method_create(node, group_id, ~0, "info",  [("topic", v.O_METHOD_PTYPE_STRING)])
# 		v.send_o_method_create(node, group_id, ~0, "login", [("nick", v.O_METHOD_PTYPE_STRING)])
# 		v.send_o_method_create(node, group_id, ~0, "logout", [])
# 		v.send_o_method_create(node, group_id, ~0, "join",  [("channel", v.O_METHOD_PTYPE_STRING)])
# 		v.send_o_method_create(node, group_id, ~0, "leave", [("channel", v.O_METHOD_PTYPE_STRING)])
# 		v.send_o_method_create(node, group_id, ~0, "say",   [("channel", v.O_METHOD_PTYPE_STRING), ("msg", v.O_METHOD_PTYPE_STRING)])
# 		print "Initializing servlet methods"
# 	else:
# 		try:	u = the_users[node]
# 		except:	u = None
# 		if u != None:
# 			print "Found tawk client, node", node
# 			if name == "tawk-client":
# 				u.methods = MethodGroup(group_id)
# 				v.send_o_method_group_subscribe(node, group_id)
# 
# def cb_o_method_create(node, group_id, method_id, name, params):
# 	global the_methods, the_avatar, the_users
# 	if node == the_avatar:
# 		if name == "info":
# 			the_methods.info = Method(method_id, name, params)
# 		elif name == "login":
# 			the_methods.login = Method(method_id, name, params)
# 		elif name == "logout":
# 			the_methods.logout = Method(method_id, name, params)
# 		elif name == "join":
# 			the_methods.join = Method(method_id, name, params)
# 		elif name == "leave":
# 			the_methods.leave = Method(method_id, name, params)
# 		elif name == "say":
# 			the_methods.say = Method(method_id, name, params)
# 	else:
# 		try:	u = the_users[node]
# 		except:	return
# 		u.add_method(method_id, name, params)
# 
# def cb_o_method_call(node, group_id, method_id, sender, params):
# 	global the_avatar, the_methods
# 	if node == the_avatar and group_id == the_methods.id:
# #		print "got", method_id, "from", sender, ":", params
# 		if method_id == the_methods.info.id:
# 			do_info(sender, params)
# 		elif method_id == the_methods.login.id:
# 			do_login(sender, params)
# 		elif method_id == the_methods.logout.id:
# 			do_logout(sender, params)
# 		elif method_id == the_methods.join.id:
# 			do_join(sender, params)
# 		elif method_id == the_methods.leave.id:
# 			do_leave(sender, params)
# 		elif method_id == the_methods.say.id:
# 			do_say(sender, params)
# 
# def cb_connect_accept(my_avatar, address, host_id):	
# 	global the_avatar, the_methods
# 	print "Connected as", my_avatar, "to", address
# 	v.send_node_name_set(my_avatar, "tawksrv")
# 	v.send_node_subscribe(my_avatar)
# 	v.send_o_method_group_create(my_avatar, ~0, "tawk")
# 	the_avatar = my_avatar
# 	v.send_node_index_subscribe(1 << v.OBJECT);
# 
# if __name__ == "__main__":
# 	the_groups = []
# 	v.callback_set(v.SEND_CONNECT_ACCEPT,		cb_connect_accept)
# 	v.callback_set(v.SEND_NODE_DESTROY,		cb_node_destroy)
# 	v.callback_set(v.SEND_O_METHOD_GROUP_CREATE,	cb_o_method_group_create)
# 	v.callback_set(v.SEND_O_METHOD_CREATE,		cb_o_method_create)
# 	v.callback_set(v.SEND_O_METHOD_CALL,		cb_o_method_call)
# 
# 	server = "localhost"
# 	for a in sys.argv[1:]:
# 		if a.startswith("-ip="):
# 			server = a[4:]
# 		elif a.startswith("-help"):
# 			print "tawkserv version %s written by Emil Brink." % VERSION
# 			print "Copyright (c) 2005 by PDC, KTH."
# 			print "Usage: tawkserv.py [-ip=IP[:HOST]] [-help] [-version]"
# 			sys.exit(1)
# 		elif a.startswith("-version"):
# 			print VERSION
# 			sys.exit(0)
# 		else:
# 			print "Ignoring unknown option \"%s\"" % a
# 	v.send_connect("tawkserv", "<secret>", server, 0)
# 
# 	while True:
# 		v.callback_update(10000)
