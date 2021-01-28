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

	$.each(subParts, function(i, item) {
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
var client = mqtt.connect({port: 9001, keepalive: 30, clean: true, resubscribe: false});

client.on("connect", function() {
	$.each(subscriptions, function(i, item) {
		client.subscribe(item.topic);
	});
});


client.on("message", function(topic, message, packet) {
	$.each(subscriptions, function(i, item) {
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
	$.each(subscriptions, function(i, item) {
		message += item.topic;
		message += " ";
	});
	console.log(message);
}

/* ========== Unsubscribe ========== */
function unsubscribe(topic) {
	subscriptions = $.map(subscriptions, function(item, i) {
		if(topic_matches_sub(topic, item.topic)) {
			client.unsubscribe(item.topic);
			return null;
		}
		return item;
	});

	var message = "Subscriptions: "
	$.each(subscriptions, function(i, item) {
		message += item.topic;
		message += " ";
	});
	console.log(message);
}