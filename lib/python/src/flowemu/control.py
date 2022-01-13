#!/usr/bin/env python3

# FlowEmu - Flow-Based Network Emulator
# Copyright (c) 2022 Institute of Communication Networks (ComNets),
#                    Hamburg University of Technology (TUHH),
#                    https://www.tuhh.de/comnets
# Copyright (c) 2022 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import paho.mqtt.client as mqtt

class Control(object):
	def __init__(self, host="localhost", port=1883):
		self.mqttc = mqtt.Client()
		self.subscriptions = []

		self.mqttc.on_connect = self._onConnect
		self.mqttc.on_message = self._onMessage

		self.mqttc.connect(host, port, 60)

		self.mqttc.loop_start()

	def _onConnect(self, client, userdata, flags, rc):
		print("Successfully connected to FlowEmu MQTT broker!")

	def _onMessage(self, client, userdata, msg):
		for subscription in self.subscriptions:
			if subscription["topic"] == msg.topic:
				subscription["callback"](msg.payload.decode("utf-8"))

	def setModuleParameter(self, module, parameter, value):
		self.mqttc.publish(f"set/module/{module}/{parameter}", value)
	
	def onModuleStatistic(self, module, statistic):
		def onModuleStatistic(function):
			topic = f"get/module/{module}/{statistic}"
			self.subscriptions.append({"topic": topic, "callback": function})
			self.mqttc.subscribe(topic)
		return onModuleStatistic

	def close(self):
		self.mqttc.loop_stop()
