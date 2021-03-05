class NodeEditor {
	element = undefined;
	svg = undefined;

	nodes = [];
	paths = [];

	dragged_element = undefined;

	constructor(div) {
		this.element = document.querySelector(div);
		this.element.classList.add("node_editor");

		this.svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
		this.element.appendChild(this.svg);

		var that = this;
		this.element.addEventListener("mousemove", function(e) {
			if (that.dragged_element instanceof Node) {
				const node_editor_position = that.element.getBoundingClientRect();

				that.dragged_element.setPosition({
					"x": e.clientX - node_editor_position.x - that.dragged_element.drag_offset.x,
					"y": e.clientY - node_editor_position.y - that.dragged_element.drag_offset.y
				});

				that.paths.forEach(function(path){
					path.update();
				});
			}

			e.stopPropagation();
		}, false);

		this.element.addEventListener("mouseup", function(e) {
			that.dragged_element = undefined;

			e.stopPropagation();
		});
	}

	addNode(node) {
		this.nodes.push(node);
		node.parent = this;

		this.element.appendChild(node.element);
	}

	addPath(path) {
		this.paths.push(path);
		path.parent = this;

		this.svg.appendChild(path.element);
	}
}

class Node {
	element = undefined;
	parent = undefined;

	drag_offset = {"x": 0, "y": 0};

	constructor() {
		this.element = document.createElement("div");
		this.element.classList.add("node");

		this.setPosition({"x": 0, "y": 0});

		var that = this;
		this.element.addEventListener("mousedown", function(e) {
			that.parent.dragged_element = that;
			that.drag_offset = {"x": e.layerX, "y": e.layerY};

			e.stopPropagation();
		});
	}

	setPosition(position) {
		this.element.style.left = position.x;
		this.element.style.top = position.y;
	}

	getPosition() {
		return {
			"x": parseInt(this.element.style.left),
			"y": parseInt(this.element.style.top)
		};
	}

	getSize() {
		return {
			"width": this.element.clientWidth,
			"height": this.element.clientHeight
		};
	}
}

class Path {
	element = undefined;
	parent = undefined;

	node_from = undefined
	node_to = undefined

	constructor(from, to) {
		this.setNodeFrom(from);
		this.setNodeTo(to);

		this.element = document.createElementNS("http://www.w3.org/2000/svg", "path");

		this.update();
	}

	setNodeFrom(from) {
		this.node_from = from;
	}

	setNodeTo(to) {
		this.node_to = to;
	}

	update() {
		const from_position = this.node_from.getPosition();
		from_position.x += this.node_from.getSize().width;
		from_position.y += this.node_from.getSize().height / 2;

		const to_position = this.node_to.getPosition();
		to_position.y += this.node_to.getSize().height / 2;

		const dist = Math.abs(to_position.x - from_position.x) / 2;

		var path_string = "";
		path_string += "M ";
		path_string += from_position.x + " " + from_position.y + " ";
		path_string += "C ";
		path_string += (from_position.x + dist) + " " + from_position.y + ", ";
		path_string += (to_position.x - dist) + " " + to_position.y + ", ";
		path_string += to_position.x + " " + to_position.y;

		this.element.setAttribute("d", path_string);
	}
}
