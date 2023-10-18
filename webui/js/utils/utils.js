const number_format = new Intl.NumberFormat("en-US", {
	"minimumFractionDigits": 3,
	"maximumFractionDigits": 3,
	"trailingZeroDisplay": "stripIfInteger"
});

function formatValue(value) {
	return number_format.format(value).replace(/,/g, "\u202f");
}

function parseFormatedValue(string) {
	return parseFloat(string.replace(/ |\u202f|,/g, ""));
}

function chartTooltip(context) {
	let label = context.dataset.label + ": " + formatValue(context.parsed.y);

	if(this.unit) {
		label += " " + this.unit;
	}

	return label;
}
