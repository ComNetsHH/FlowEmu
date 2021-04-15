class NodeEditor {
	element = undefined;
	svg = undefined;

	nodes = {};
	paths = [];
	loose_path = undefined;

	mouse_position = {"x": 0, "y": 0};
	selected_element = undefined;
	dragged_element = undefined;

	node_add_handler = undefined;
	node_change_handler = undefined;
	node_remove_handler = undefined;
	path_add_handler = undefined;
	path_remove_handler = undefined;

	constructor(div) {
		this.element = document.querySelector(div);
		this.element.classList.add("node_editor");

		this.svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
		this.element.appendChild(this.svg);

		var that = this;
		window.addEventListener("mousedown", function(e) {
			if(that.selected_element !== undefined) {
				that.selected_element.unselect();
			}

			if(that.loose_path !== undefined) {
				that.removePath(that.loose_path);
			}

			e.stopPropagation();
		});

		window.addEventListener("mousemove", function(e) {
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

			if(that.loose_path !== undefined) {
				that.loose_path.update();
			}

			e.stopPropagation();
		}, false);

		window.addEventListener("mouseup", function(e) {
			if(that.dragged_element instanceof Node) {
				if(that.node_change_handler !== undefined) {
					that.node_change_handler(that.dragged_element);
				}
			}

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

	addNode(node, call_callback = true) {
		if(call_callback && this.node_add_handler !== undefined) {
			this.node_add_handler(node);
		}

		this.nodes[node.id] = node;
		node.parent = this;

		this.element.appendChild(node.element);

		var that = this;
		node.element.addEventListener("mousedown", function(e) {
			if(that.selected_element !== undefined) {
				that.selected_element.unselect();
			}
			node.select();

			that.dragged_element = node;
			node.drag_offset = {"x": e.layerX, "y": e.layerY};

			e.stopPropagation();
		});
	}

	updateNode(node_id, node_data, call_callback = true) {
		if(!(node_id in this.nodes)) {
			log.error("Node with ID " + node_id + " does not exist!");
			return;
		}

		var node = this.nodes[node_id];
		var modified = false;

		if(node.getPosition().x != node_data.position.x   ||
		   node.getPosition().y != node_data.position.y   ||
		   node.getSize().width != node_data.size.width   ||
		   node.getSize().height != node_data.size.height   ) {
			node.setPosition(node_data.position);
			node.setSize(node_data.size);

			this.paths.forEach(function(path) {
				if(path.port_from.parent.parent === node || path.port_to.parent.parent === node) {
					path.update();
				}
			});

			modified = true;
		}

		if(call_callback && modified && this.node_change_handler !== undefined) {
			this.node_change_handler(node);
		}
	}

	removeNode(node, call_callback = true) {
		if(call_callback && this.node_remove_handler !== undefined) {
			this.node_remove_handler(node);
		}

		if(this.selected_element === node) {
			this.selected_element = undefined;
		}

		if(this.dragged_element === node) {
			this.dragged_element = undefined;
		}

		var that = this;
		this.paths.forEach(function(path) {
			if(path.port_from.parent.parent === node || path.port_to.parent.parent === node) {
				that.removePath(path);
			}
		});

		node.element.remove();
		delete this.nodes[node.id];
	}

	addPath(path, call_callback = true) {
		this.paths.push(path);
		path.parent = this;

		this.svg.appendChild(path.element);

		if(path.mouse === undefined) {
			if(call_callback && this.path_add_handler !== undefined) {
				this.path_add_handler(path);
			}
		}
	}

	updatePathFrom(path, from, call_callback = true) {
		var old_path_mouse = path.mouse;

		path.setPortFrom(from);

		if(old_path_mouse == "from" && from !== "mouse") {
			if(call_callback && this.path_add_handler !== undefined) {
				this.path_add_handler(path);
			}
		} else if(from === "mouse") {
			if(call_callback && this.path_remove_handler !== undefined) {
				this.path_remove_handler(path);
			}
		}
	}

	updatePathTo(path, to, call_callback = true) {
		var old_path_mouse = path.mouse;

		path.setPortTo(to);

		if(old_path_mouse == "to" && to !== "mouse") {
			if(call_callback && this.path_add_handler !== undefined) {
				this.path_add_handler(path);
			}
		} else if(to === "mouse") {
			if(call_callback && this.path_remove_handler !== undefined) {
				this.path_remove_handler(path);
			}
		}
	}

	updatePaths(paths_data, call_callback = true) {
		var that = this;

		this.paths.forEach(function(path1) {
			var found = false;

			var path1_data = path1.serialize();

			if(path1_data.from === null || path1_data.to === null) {
				return;
			}

			paths_data.forEach(function(path2_data) {
				if(path1_data.from.node === path2_data.from.node &&
				   path1_data.from.port === path2_data.from.port &&
				   path1_data.to.node === path2_data.to.node &&
				   path1_data.to.port === path2_data.to.port) {
					found = true;
				}
			});

			if(!found) {
				that.removePath(path1, call_callback);
			}
		});

		paths_data.forEach(function(path1_data) {
			var found = false;

			that.paths.forEach(function(path2) {
				var path2_data = path2.serialize();

				if(path2_data.from === null || path2_data.to === null) {
					return;
				}

				if(path1_data.from.node === path2_data.from.node &&
				   path1_data.from.port === path2_data.from.port &&
				   path1_data.to.node === path2_data.to.node &&
				   path1_data.to.port === path2_data.to.port) {
					found = true;
				}
			});

			if(!found) {
				if(that.nodes[path1_data.from.node] === undefined ||
				   that.nodes[path1_data.from.node].ports[path1_data.from.port] === undefined ||
				   that.nodes[path1_data.to.node] === undefined ||
				   that.nodes[path1_data.to.node].ports[path1_data.to.port] === undefined) {
					return;
				}

				let from = that.nodes[path1_data.from.node].ports[path1_data.from.port];
				let to = that.nodes[path1_data.to.node].ports[path1_data.to.port];

				that.addPath(new Path(from, to), call_callback);
			}
		});
	}

	removePath(path, call_callback = true) {
		if(path.mouse === undefined) {
			if(call_callback && this.path_remove_handler !== undefined) {
				this.path_remove_handler(path);
			}
		}

		path.element.remove();
		this.paths = this.paths.filter(item => item !== path);

		if(path.port_from !== undefined) {
			path.port_from.connected_path = undefined;
		}

		if(path.port_to !== undefined) {
			path.port_to.connected_path = undefined;
		}

		if(this.loose_path === path) {
			this.loose_path = undefined;
		}

		path = undefined;
	}

	setNodeAddHandler(handler) {
		this.node_add_handler = handler;
	}

	setNodeChangeHandler(handler) {
		this.node_change_handler = handler;
	}

	setNodeRemoveHandler(handler) {
		this.node_remove_handler = handler;
	}

	setPathAddHandler(handler) {
		this.path_add_handler = handler;
	}

	setPathRemoveHandler(handler) {
		this.path_remove_handler = handler;
	}

	serializeNodes() {
		var data = {};
		for(let key in this.nodes) {
			data[key] = this.nodes[key].serialize();
		}

		return data;
	}

	serializePaths() {
		var data = [];
		this.paths.forEach(function(path) {
			if(path.mouse === undefined) {
				data.push(path.serialize());
			}
		});

		return data;
	}

	serialize() {
		var data = {
			"nodes": this.serializeNodes(),
			"paths": this.serializePaths()
		};

		return data;
	}
}

class Node {
	id = undefined;

	element = undefined;
	header = undefined;
	content = undefined;

	parent = undefined;
	type = undefined;
	title = undefined;
	content_items = [];
	ports = {};

	drag_offset = {"x": 0, "y": 0};

	constructor(id) {
		if(id !== undefined) {
			this.id = id;
		} else {
			this.id = Date.now();
		}

		this.element = document.createElement("div");
		this.element.classList.add("node");

		this.header = document.createElement("div");
		this.header.classList.add("header");
		this.element.appendChild(this.header);

		this.content = document.createElement("ul");
		this.content.classList.add("content");
		this.element.appendChild(this.content);

		this.setPosition({"x": 0, "y": 0});
	}

	setType(type) {
		this.type = type;
	}

	getType() {
		return this.type;
	}

	setTitle(title) {
		this.title = title;
		this.header.innerHTML = title;
	}

	getTitle() {
		return this.title;
	}

	addContentItem(item) {
		this.content_items.push(item);
		item.parent = this;

		if(item instanceof NodeContentFlow) {
			var that = this;
			item.ports_left.forEach(function(port) {
				that.ports[port.getId()] = port;
			});

			item.ports_right.forEach(function(port) {
				that.ports[port.getId()] = port;
			});
		}

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

	setSize(size) {
		this.element.style.minWidth = size.width;
		this.element.style.minHeight = size.height;
	}

	getSize() {
		return {
			"width": parseInt(this.element.clientWidth),
			"height": parseInt(this.element.clientHeight)
		};
	}

	select() {
		this.parent.selected_element = this;

		this.element.classList.add("selected");
	}

	unselect() {
		this.element.classList.remove("selected");
	}

	serialize() {
		var data = {
			"type": this.getType(),
			"title": this.getTitle(),
			"position": this.getPosition(),
			"size": this.getSize(),
			"content": []
		};

		this.content_items.forEach(function(item) {
			data.content.push(item.serialize());
		});

		return data;
	}

	deserialize(node_data) {
		this.setType(node_data.type);
		this.setTitle(node_data.title);
		this.setPosition(node_data.position);

		var that = this;
		node_data.content.forEach(function(item) {
			switch(item.type) {
				case "label":
					var label = new NodeContentLabel(item.label);
					that.addContentItem(label);
					break;
				case "flow":
					var flow = new NodeContentFlow();
					item.ports.left.forEach(function(port_data) {
						var port = new Port(port_data.id, port_data.type, port_data.label);
						flow.addPort("left", port);
					});
					item.ports.right.forEach(function(port_data) {
						var port = new Port(port_data.id, port_data.type, port_data.label);
						flow.addPort("right", port);
					});
					that.addContentItem(flow);
					break;
			}
		});
	}
}

class NodeContentItem {
	element = undefined;

	parent = undefined;

	constructor() {
		this.element = document.createElement("li");
	}

	serialize() {
		var data = {
			"type": null
		};

		return data;
	}
}

class NodeContentLabel extends NodeContentItem {
	label = undefined;

	constructor(label) {
		super();
		this.element.classList.add("label");

		this.label = label;
		this.element.innerHTML = label;
	}

	getLabel() {
		return this.label;
	}

	serialize() {
		var data = {
			"type": "label",
			"label": this.getLabel()
		};

		return data;
	}
}

class NodeContentFlow extends NodeContentItem {
	left = undefined;
	right = undefined;

	ports_left = [];
	ports_right = [];

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
				this.ports_left.push(port);
				if(this.parent !== undefined) {
					this.parent.ports[port.id] = port;
				}
				this.left.appendChild(port.element);
				break;
			case "right":
				this.ports_right.push(port);
				if(this.parent !== undefined) {
					this.parent.ports[port.id] = port;
				}
				this.right.appendChild(port.element);
				break;
		}
	}

	serialize() {
		var data = {
			"type": "flow",
			"ports": {
				"left": [],
				"right": []
			}
		};

		this.ports_left.forEach(function(port) {
			data.ports.left.push(port.serialize());
		});

		this.ports_right.forEach(function(port) {
			data.ports.right.push(port.serialize());
		});

		return data;
	}
}

class Port {
	id = undefined;

	element = undefined;

	parent = undefined;
	type = undefined;
	label = undefined;
	side = undefined;
	connected_path = undefined;

	constructor(id, type, label) {
		this.id = id;

		this.element = document.createElement("div");
		this.element.classList.add("port");

		this.setType(type);

		this.label = label;
		this.element.innerHTML = label;

		var that = this;
		this.element.addEventListener("mousedown", function(e) {
			if(that.connected_path === undefined) {
				if(that.getNodeEditor().loose_path === undefined) {
					var path = new Path(that, "mouse");
					that.getNodeEditor().addPath(path);
					that.getNodeEditor().loose_path = path;
					that.getNodeEditor().loose_path.update();
				} else {
					if(that.getNodeEditor().loose_path.port_from === undefined) {
						that.getNodeEditor().updatePathFrom(that.getNodeEditor().loose_path, that);
						that.getNodeEditor().loose_path.update();
						that.getNodeEditor().loose_path = undefined;
					} else if(that.getNodeEditor().loose_path.port_to === undefined) {
						that.getNodeEditor().updatePathTo(that.getNodeEditor().loose_path, that);
						that.getNodeEditor().loose_path.update();
						that.getNodeEditor().loose_path = undefined;
					}
				}
			} else if(that.getNodeEditor().loose_path === undefined) {
				if(that.connected_path.port_from === that) {
					that.getNodeEditor().loose_path = that.connected_path;
					that.getNodeEditor().updatePathFrom(that.getNodeEditor().loose_path, "mouse");
					that.getNodeEditor().loose_path.update();
				} else if(that.connected_path.port_to === that) {
					that.getNodeEditor().loose_path = that.connected_path;
					that.getNodeEditor().updatePathTo(that.getNodeEditor().loose_path, "mouse");
					that.getNodeEditor().loose_path.update();
				}
			}

			e.stopPropagation();
		});
	}

	getNodeEditor() {
		return this.parent.parent.parent;
	}

	getId() {
		return this.id;
	}

	setType(type) {
		this.type = type;

		switch(type) {
			case "sending":
				this.element.classList.remove("receiving", "requesting", "responding");
				this.element.classList.add("sending");
				break;
			case "receiving":
				this.element.classList.remove("sending", "requesting", "responding");
				this.element.classList.add("receiving");
				break;
			case "requesting":
				this.element.classList.remove("sending", "receiving", "responding");
				this.element.classList.add("requesting");
				break;
			case "responding":
				this.element.classList.remove("sending", "receiving", "requesting");
				this.element.classList.add("responding");
				break;
		}
	}

	getType() {
		return this.type;
	}

	getLabel() {
		return this.label;
	}

	getPosition() {
		const rect = this.element.getBoundingClientRect();
		const rectNodeEditor = this.getNodeEditor().element.getBoundingClientRect();

		const postion_x = rect.left - rectNodeEditor.left;
		const postion_y = rect.top - rectNodeEditor.top;

		return {
			"x": ((this.side === "right") ? postion_x + this.element.clientWidth : postion_x),
			"y": postion_y + this.element.clientHeight / 2
		};
	}

	serialize() {
		var data = {
			"id": this.getId(),
			"type": this.getType(),
			"label": this.getLabel(),
		};

		return data;
	}
}

class Path {
	element = undefined;
	parent = undefined;

	mouse = undefined
	port_from = undefined
	port_to = undefined

	constructor(from, to) {
		if(from === "mouse" && to === "mouse") {
			console.error("Cannot create a path from mouse to mouse!");
		}

		this.setPortFrom(from);
		this.setPortTo(to);

		this.element = document.createElementNS("http://www.w3.org/2000/svg", "path");

		this.update();
	}

	setPortFrom(from) {
		if(from === "mouse") {
			this.mouse = "from";
			if(this.port_from !== undefined) {
				this.port_from.connected_path = undefined;
				this.port_from = undefined;
			}
		} else {
			if(this.mouse === "from") {
				this.mouse = undefined;
			}
			from.connected_path = this;
			this.port_from = from;
		}
	}

	setPortTo(to) {
		if(to === "mouse") {
			this.mouse = "to";
			if(this.port_to !== undefined) {
				this.port_to.connected_path = undefined;
				this.port_to = undefined;
			}
		} else {
			if(this.mouse === "to") {
				this.mouse = undefined;
			}
			to.connected_path = this;
			this.port_to = to;
		}
	}

	update() {
		if(this.mouse !== undefined && this.parent === undefined) {
			return;
		}

		const from_position = (this.mouse === "from" ? this.parent.mouse_position : this.port_from.getPosition());
		const to_position = (this.mouse === "to" ? this.parent.mouse_position : this.port_to.getPosition());

		const dist = Math.max(Math.abs(to_position.x - from_position.x) / 2, Math.min(Math.abs(to_position.y - from_position.y) * 2, 100));
		const dist_from = ((this.port_from !== undefined && this.port_from.side === "left") || (this.port_from === undefined && this.port_to !== undefined && this.port_to.side === "right") ? (0 - dist) : dist)
		const dist_to = ((this.port_to !== undefined && this.port_to.side === "left") || (this.port_to === undefined && this.port_from !== undefined && this.port_from.side === "right") ? (0 - dist) : dist)

		var path_string = "";
		path_string += "M ";
		path_string += from_position.x + " " + from_position.y + " ";
		path_string += "C ";
		path_string += (from_position.x + dist_from) + " " + from_position.y + ", ";
		path_string += (to_position.x + dist_to) + " " + to_position.y + ", ";
		path_string += to_position.x + " " + to_position.y;

		this.element.setAttribute("d", path_string);
	}

	serialize() {
		var data = {
			"from": (this.port_from !== undefined ? {"node": this.port_from.parent.parent.id, "port": this.port_from.id} : null),
			"to": (this.port_to !== undefined ? {"node": this.port_to.parent.parent.id, "port": this.port_to.id} : null)
		};

		return data;
	}
}

class NodeLibrary {
	element = undefined;
	header = undefined;
	content = undefined;

	node_editor = undefined;
	groups = [];

	constructor(div) {
		this.element = document.querySelector(div);
		this.element.classList.add("node_library");
	}

	connectNodeEditor(node_editor) {
		this.node_editor = node_editor;
	}

	addGroup(group) {
		this.groups.push(group);
		group.parent = this;

		this.element.appendChild(group.element);
	}
}

class NodeLibraryGroup {
	element = undefined;

	parent = undefined;
	title = undefined;
	nodes = {}

	constructor(title) {
		this.title = title;

		this.element = document.createElement("div");
		this.element.classList.add("group");

		this.header = document.createElement("div");
		this.header.classList.add("header");
		this.header.innerHTML = title;
		this.element.appendChild(this.header);

		this.content = document.createElement("ul");
		this.content.classList.add("content");
		this.element.appendChild(this.content);

		var that = this;
		this.header.addEventListener("click", function(e) {
			if(that.element.classList.contains("collapsed")) {
				that.element.classList.remove("collapsed");
			} else {
				that.element.classList.add("collapsed");
			}
		});
	}

	addNode(node) {
		this.nodes[node.id] = node;
		node.parent = this;

		this.content.appendChild(node.element);

		var that = this;
		node.element.addEventListener("mousedown", function(e) {
			if(that.parent.node_editor === undefined) {
				return;
			}

			let library_node_position = node.element.getBoundingClientRect();
			let node_editor_position = that.parent.node_editor.element.getBoundingClientRect();

			var new_node = new Node();
			new_node.deserialize(node.serialize());
			new_node.setPosition({
				"x": library_node_position.x - node_editor_position.x,
				"y": library_node_position.y - node_editor_position.y
			});
			that.parent.node_editor.addNode(new_node);

			if(that.parent.node_editor.selected_element !== undefined) {
				that.parent.node_editor.selected_element.unselect();
			}
			new_node.select();

			that.parent.node_editor.dragged_element = new_node;
			new_node.drag_offset = {"x": e.x - library_node_position.x, "y": e.y - library_node_position.y};

			e.stopPropagation();
		});
	}
}
