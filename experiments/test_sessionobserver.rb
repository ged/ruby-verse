sess_observer_class = Class.new do
	include Verse::SessionObserver,
	        Verse::Loggable

	attr_accessor :callback_values

	def on_connect_accept( avatar, address, host_id )
		self.log.notice "Got connect_accept: %p" % [[ avatar, address, host_id ]]
		self.callback_values = [avatar, address, host_id]
	end
end
