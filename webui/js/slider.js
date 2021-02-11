$(function() {
	$(".parameter").each(function() {
		var box = $(this);
		var label = $(this).children(".parameter_value").first();
		var slider = $(this).children(".parameter_slider").first();

		subscribe("get/" + box.attr("parameter"), function(topic, payload) {
			var value = parseInt(payload);

			label.text(value);
			slider.data("current_value", value);

			if(slider.data("clicked") != true) {
				slider.val(value);
			}
		});

		slider.on("input", function() {
			publish("set/" + box.attr("parameter"), $(this).val());
			$(this).data("clicked", true);

			var timeout = $(this).data("timeout");
			clearTimeout(timeout);
			timeout = setTimeout(function(element) {
				element.data("clicked", false);
				element.val(element.data("current_value"));
			}, 1000, $(this));
			$(this).data("timeout", timeout);
		});
	});
});
