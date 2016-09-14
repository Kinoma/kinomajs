// RFC7540 5.1

const SendH = "SendH";
const RecvH = "RecvH";
const SendPP = "SendPP";
const RecvPP = "RecvPP";
const SendES = "SendES";
const RecvES = "RecvES";
const SendR = "SendR";
const RecvR = "RecvR";

export const Event = {
	SendH,
	RecvH,
	SendPP,
	RecvPP,
	SendES,
	RecvES,
	SendR,
	RecvR
};

// State Machine

class State_Base {
	toDebugString() {
		return '[State:' + this.label + ']';
	}

	get active() {
		return false;
	}

	action(event) {
		switch (event) {
			case SendR:
			case RecvR:
				return Closed;

			default:
				this.deny(event);
		}
	}

	deny(event) {
		throw 'Invalid action "' + event + '" for state "' + this.label + '"';
	}
};

class State_Active extends State_Base {
	get active() {
		return true;
	}
}

class State_Idle extends State_Base {
	get label() {
		return "Idle";
	}

	action(event) {
		switch (event) {
			case SendH:
			case RecvH:
				return Open;

			case SendPP:
				return ReservedLocal;

			case RecvPP:
				return ReservedRemote;

			default:
				this.deny(event);
		}
	}
};

class State_ReservedLocal extends State_Base {
	get label() {
		return "ReservedLocal";
	}

	action(event) {
		switch (event) {
			case SendH:
				return HalfClosedRemote;

			default:
				return super.action(event);
		}
	}
}

class State_ReservedRemote extends State_Base {
	get label() {
		return "ReservedRemote";
	}

	action(event) {
		switch (event) {
			case RecvH:
				return HalfClosedLocal;

			default:
				return super.action(event);
		}
	}
}

class State_Open extends State_Active {
	get label() {
		return "Open";
	}

	action(event) {
		switch (event) {
			case SendES:
				return HalfClosedLocal;

			case RecvES:
				return HalfClosedRemote;

			default:
				return super.action(event);
		}
	}
}

class State_HalfClosedRemote extends State_Active {
	get label() {
		return "HalfClosedRemote";
	}

	action(event) {
		switch (event) {
			case SendES:
				return Closed;

			default:
				return super.action(event);
		}
	}
}

class State_HalfClosedLocal extends State_Active {
	get label() {
		return "HalfClosedLocal";
	}

	action(event) {
		switch (event) {
			case RecvES:
				return Closed;

			default:
				return super.action(event);
		}
	}
}

class State_Closed extends State_Base {
	get label() {
		return "Closed";
	}

	action(event) {
		return this;
	}
}

const Idle = new State_Idle();
const ReservedLocal = new State_ReservedLocal();
const ReservedRemote = new State_ReservedRemote();
const Open = new State_Open();
const HalfClosedRemote = new State_HalfClosedRemote();
const HalfClosedLocal = new State_HalfClosedLocal();
const Closed = new State_Closed();

export const State = {
	Idle,
	ReservedLocal,
	ReservedRemote,
	Open,
	HalfClosedRemote,
	HalfClosedLocal,
	Closed
};

export default {
	State,
	Event
};
