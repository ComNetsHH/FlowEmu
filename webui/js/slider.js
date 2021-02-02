$(function(){
	// Delay
	subscribe("get/fixed_delay/delay", function(topic, payload) {
		var value = parseInt(payload);

		$("#delay_value").text(value);
		$("#delay_slider").data("current_value", value);

		if($("#delay_slider").data("clicked") != true) {
			$("#delay_slider").val(value);
		}
	});

	$("#delay_slider").on("input", function() {
		publish("set/fixed_delay/delay", $(this).val());
		$(this).data("clicked", true);

		var timeout = $(this).data("timeout");
		clearTimeout(timeout);
		timeout = setTimeout( function(element) {
			element.data("clicked", false);
			element.val(element.data("current_value"));
		}, 1000, $(this));
		$(this).data("timeout", timeout);
	});

	// Loss
	subscribe("get/uncorrelated_loss/loss_probability", function(topic, payload) {
		var value_p = parseFloat(payload);
		var value = parseInt(value_p * 10000);

		$("#loss_value").text(value_p * 100);
		$("#loss_slider").data("current_value", value);

		if($("#loss_slider").data("clicked") != true) {
			$("#loss_slider").val(value);
		}
	});

	$("#loss_slider").on("input", function() {
		publish("set/uncorrelated_loss/loss_probability", ($(this).val() / 10000).toString());
		$(this).data("clicked", true);

		var timeout = $(this).data("timeout");
		clearTimeout(timeout);
		timeout = setTimeout( function(element) {
			element.data("clicked", false);
			element.val(element.data("current_value"));
		}, 1000, $(this));
		$(this).data("timeout", timeout);
	});
});
