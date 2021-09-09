/*
 * FlowEmu - Flow-Based Network Emulator
 * Copyright (c) 2021 Institute of Communication Networks (ComNets),
 *                    Hamburg University of Technology (TUHH),
 *                    https://www.tuhh.de/comnets
 * Copyright (c) 2021 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

//
// Originally created by Daniel Stolpmann as part of his private home automation system.
//

function subscription(topic, callback) {
	this.topic = topic
	this.callback = callback
}

var subscriptions = [];

function topic_matches_sub(sub, topic) {
	subParts = sub.split("/");
	topicParts = topic.split("/");

	var result = (subParts.length == topicParts.length);

	subParts.forEach(function(item, i) {
		if(item == topicParts[i]) {
			return;
		} else if(item == "+") {
			return;
		} else if(item == "#") {
			result = true
			return false;
		} else {
			result = false
			return false;
		}
	});

	return result;
}

/* ========== Connect ========== */
var client = mqtt.connect({keepalive: 30, clean: true, resubscribe: false});

client.on("connect", function() {
	subscriptions.forEach(function(item) {
		client.subscribe(item.topic);
	});
});


client.on("message", function(topic, message, packet) {
	subscriptions.forEach(function(item) {
		if(topic_matches_sub(item.topic, topic)) {
			item.callback(topic, message);
		}
	});
});

/* ========== Publish ========== */
function publish(topic, payload) {
	if(typeof payload === "object") {
		payload = JSON.stringify(payload);
	}

	var message = payload.replace("%27", "'");
	client.publish(topic, message);
}

/* ========== Subscribe ========== */
function subscribe(topic, callback) {
	subscriptions.push(new subscription(topic, callback));

	if(client.connected) {
		client.subscribe(topic);
	}

	var message = "Subscriptions: "
	subscriptions.forEach(function(item) {
		message += item.topic;
		message += " ";
	});
	console.log(message);
}

/* ========== Unsubscribe ========== */
function unsubscribe(topic) {
	subscriptions = subscriptions.filter(function(item) {
		if(topic_matches_sub(topic, item.topic)) {
			client.unsubscribe(item.topic);
			return false;
		}
		return true;
	});

	var message = "Subscriptions: "
	subscriptions.forEach(function(item) {
		message += item.topic;
		message += " ";
	});
	console.log(message);
}
