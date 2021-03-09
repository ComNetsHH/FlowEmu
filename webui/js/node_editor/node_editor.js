class NodeEditor {
	element = undefined;
	svg = undefined;

	nodes = [];
	paths = [];

	mouse_position = {"x": 0, "y": 0};
	selected_element = undefined;
	dragged_element = undefined;

	constructor(div) {
		this.element = document.querySelector(div);
		this.element.classList.add("node_editor");

		this.svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
		this.element.appendChild(this.svg);

		var that = this;
		this.element.addEventListener("mousedown", function(e) {
			if(that.selected_element !== undefined) {
				that.selected_element.unselect();
			}

			e.stopPropagation();
		});

		this.element.addEventListener("mousemove", function(e) {
			const node_editor_position = that.element.getBoundingClientRect();
			that.mouse_position = {"x": e.clientX - node_editor_position.x, "y": e.clientY - node_editor_position.y};

			if(that.dragged_element instanceof Node) {
				that.dragged_element.setPosition({
					"x": that.mouse_position.x - that.dragged_element.drag_offset.x,
					"y": that.mouse_position.y - that.dragged_element.drag_offset.y
				});

				that.paths.forEach(function(path) {
					path.update();
				});
			}

			e.stopPropagation();
		}, false);

		this.element.addEventListener("mouseup", function(e) {
			that.dragged_element = undefined;

			e.stopPropagation();
		});

		document.addEventListener("keydown", function(e) {
			if(e.key === "Delete") {
				if(that.selected_element instanceof Node) {
					that.removeNode(that.selected_element);
				}

				e.stopPropagation();
			}
		});
	}

	addNode(node) {
		this.nodes.push(node);
		node.parent = this;

		this.element.appendChild(node.element);
	}

	removeNode(node) {
		if(this.selected_element === node) {
			this.selected_element = undefined;
		}

		if(this.dragged_element === node) {
			this.dragged_element = undefined;
		}

		node.element.remove();
		this.nodes = this.nodes.filter(item => item !== node);

		var that = this;
		this.paths.forEach(function(path) {
			if(path.port_from.parent.parent === node || path.port_to.parent.parent === node) {
				that.removePath(path);
			}
		});

		node = undefined;
	}

	addPath(path) {
		this.paths.push(path);
		path.parent = this;

		this.svg.appendChild(path.element);
	}

	removePath(path) {
		path.element.remove();
		this.paths = this.paths.filter(item => item !== path);

		path = undefined;
	}
}

class Node {
	element = undefined;
	header = undefined;
	content = undefined;

	parent = undefined;
	content_items = [];

	drag_offset = {"x": 0, "y": 0};

	constructor() {
		this.element = document.createElement("div");
		this.element.classList.add("node");

		this.header = document.createElement("div");
		this.header.classList.add("header");
		this.element.appendChild(this.header);

		this.content = document.createElement("ul");
		this.content.classList.add("content");
		this.element.appendChild(this.content);

		this.setPosition({"x": 0, "y": 0});

		var that = this;
		this.element.addEventListener("mousedown", function(e) {
			if(that.parent.selected_element !== undefined) {
				that.parent.selected_element.unselect();
			}
			that.select();

			that.parent.dragged_element = that;
			that.drag_offset = {"x": e.layerX, "y": e.layerY};

			e.stopPropagation();
		});
	}

	setTitle(title) {
		this.header.innerHTML = title;
	}

	addContentItem(item) {
		this.content_items.push(item);
		item.parent = this;

		this.content.appendChild(item.element);
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

	select() {
		this.parent.selected_element = this;

		this.element.classList.add("selected");
	}

	unselect() {
		this.element.classList.remove("selected");
	}
}

class NodeContentItem {
	element = undefined;

	parent = undefined;

	constructor() {
		this.element = document.createElement("li");
	}
}

class NodeContentLabel extends NodeContentItem {
	constructor(text) {
		super();
		this.element.classList.add("label");

		this.element.innerHTML = text;
	}
}

class NodeContentFlow extends NodeContentItem {
	left = undefined;
	right = undefined;

	constructor() {
		super();
		this.element.classList.add("flow");

		this.left = document.createElement("div");
		this.left.classList.add("left");
		this.element.appendChild(this.left);

		var center = document.createElement("div");
		center.classList.add("center");
		this.element.appendChild(center);

		this.right = document.createElement("div");
		this.right.classList.add("right");
		this.element.appendChild(this.right);
	}

	addPort(side, port) {
		port.parent = this;
		port.side = side;

		switch(side) {
			case "left":
				this.left.appendChild(port.element);
				break;
			case "right":
				this.right.appendChild(port.element);
				break;
		}
	}
}

class Port {
	element = undefined;

	parent = undefined;

	side = undefined;

	constructor(label) {
		this.element = document.createElement("div");
		this.element.classList.add("port");
		this.element.innerHTML = label;
	}

	getPosition() {
		const rect = this.element.getBoundingClientRect();
		const rectNodeEditor = this.parent.parent.parent.element.getBoundingClientRect();

		const postion_x = rect.left - rectNodeEditor.left;
		const postion_y = rect.top - rectNodeEditor.top;

		return {
			"x": ((this.side === "right") ? postion_x + this.element.clientWidth : postion_x),
			"y": postion_y + this.element.clientHeight / 2
		};
	}
}

class Path {
	element = undefined;
	parent = undefined;

	port_from = undefined
	port_to = undefined

	constructor(from, to) {
		this.setPortFrom(from);
		this.setPortTo(to);

		this.element = document.createElementNS("http://www.w3.org/2000/svg", "path");

		this.update();
	}

	setPortFrom(from) {
		this.port_from = from;
	}

	setPortTo(to) {
		this.port_to = to;
	}

	update() {
		const from_position = this.port_from.getPosition();
		const to_position = this.port_to.getPosition();

		const dist = Math.max(Math.abs(to_position.x - from_position.x) / 2, 100);
		const dist_from = ((this.port_from.side === "left") ? (0 - dist) : dist)
		const dist_to = ((this.port_to.side === "left") ? (0 - dist) : dist)

		var path_string = "";
		path_string += "M ";
		path_string += from_position.x + " " + from_position.y + " ";
		path_string += "C ";
		path_string += (from_position.x + dist_from) + " " + from_position.y + ", ";
		path_string += (to_position.x + dist_to) + " " + to_position.y + ", ";
		path_string += to_position.x + " " + to_position.y;

		this.element.setAttribute("d", path_string);
	}
}
