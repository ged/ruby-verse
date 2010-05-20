#!/usr/bin/env ruby

require 'verse'

# The Verse session class.
class Verse::Session
	include Verse::Loggable

	### Process any pending events for this session.
	def callback_update( timeout=0.1 )
		self.class.synchronize { Verse.callback_update(timeout) }
	end

end # class Verse::Session

