/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
function XSCNode() {
	this.line = getLine();
}
XSCNode.prototype = Object.create(Object.prototype, {
	name: { value: "?", enumerable: true, writable: true },
	toJSON: {
		value: function() {
			var result = {};
			for (var i in this) {
				result[i] = this[i];
			}
			return result;
		}, writable: true
	},
});

function XSCNodes() {
	this.items = null;
}
XSCNodes.prototype = Object.create(XSCNode.prototype, {
	append: {
		value: function(node) {
			if (this.items)
				this.items.push(node);
			else
				this.items = [ node ];
		}
	},
});

/* EXPRESSIONS */

function XSCExpressions() {
	XSCNodes.call(this);
}
XSCExpressions.prototype = Object.create(XSCNodes.prototype, {
	name: { value: "Expressions", enumerable: true },
});
function XSCExpression() {
	XSCNode.call(this);
}
XSCExpression.prototype = Object.create(XSCNode.prototype, {
});

/* LITERALS */

function XSCLiteral(value) {
	XSCExpression.call(this);
	this.value = value;
}
XSCLiteral.prototype = Object.create(XSCExpression.prototype, {
});
function XSCBoolean(value) {
	XSCLiteral.call(this, value);
}
XSCBoolean.prototype = Object.create(XSCLiteral.prototype, {
	name: { value: "Boolean", enumerable: true },
});
function XSCInteger(value) {
	XSCLiteral.call(this, value);
}
XSCInteger.prototype = Object.create(XSCLiteral.prototype, {
	name: { value: "Integer", enumerable: true },
});
function XSCNull() {
	XSCLiteral.call(this, null);
}
XSCNull.prototype = Object.create(XSCLiteral.prototype, {
	name: { value: "Null", enumerable: true },
});
function XSCNumber(value) {
	XSCLiteral.call(this, value);
}
XSCNumber.prototype = Object.create(XSCLiteral.prototype, {
	name: { value: "Number", enumerable: true },
});
function XSCRegexp(value, flags) {
	XSCLiteral.call(this, value);
	this.flags = flags;
}
XSCRegexp.prototype = Object.create(XSCLiteral.prototype, {
	name: { value: "Regexp", enumerable: true },
});
function XSCString(value) {
	XSCLiteral.call(this, value);
}
XSCString.prototype = Object.create(XSCLiteral.prototype, {
	name: { value: "String", enumerable: true },
});
function XSCUndefined() {
	XSCLiteral.call(this, undefined);
}
XSCUndefined.prototype = Object.create(XSCLiteral.prototype, {
	name: { value: "Undefined", enumerable: true },
});

/* INITIALIZERS */

function XSCArray() {
	XSCNodes.call(this);
}
XSCArray.prototype = Object.create(XSCNodes.prototype, {
	name: { value: "Array", enumerable: true },
});
function XSCObject() {
	XSCNodes.call(this);
}
XSCObject.prototype = Object.create(XSCNodes.prototype, {
	name: { value: "Object", enumerable: true },
});
function XSCProperty(id, flag, value) {
	XSCNode.call(this);
	this.id = id;
	this.flag = flag;
	this.value = value;
}
XSCProperty.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Property", enumerable: true },
});
function XSCPropertyAt(at, flag, value) {
	XSCNode.call(this);
	this.at = at;
	this.flag = flag;
	this.value = value;
}
XSCPropertyAt.prototype = Object.create(XSCProperty.prototype, {
	name: { value: "PropertyAt", enumerable: true },
});

/* REFERENCES */

function XSCIdentifier(id) {
	XSCExpression.call(this);
	this.id = id;
}
XSCIdentifier.prototype = Object.create(XSCExpression.prototype, {
	name: { value: "Identifier", enumerable: true },
});
function XSCMember(reference, id) {
	XSCExpression.call(this);
	this.reference = reference;
	this.id = id;
}
XSCMember.prototype = Object.create(XSCExpression.prototype, {
	name: { value: "Member", enumerable: true },
});
function XSCMemberAt(reference, at) {
	XSCExpression.call(this);
	this.reference = reference;
	this.at = at;
}
XSCMemberAt.prototype = Object.create(XSCExpression.prototype, {
	name: { value: "MemberAt", enumerable: true },
});
function XSCThis() {
	XSCExpression.call(this);
}
XSCThis.prototype = Object.create(XSCExpression.prototype, {
	name: { value: "This", enumerable: true },
});

/* CALLS */

function XSCCall(reference, params) {
	XSCExpression.call(this);
	this.reference = reference;
	this.params = params;
}
XSCCall.prototype = Object.create(XSCExpression.prototype, {
	name: { value: "Call", enumerable: true },
});
function XSCNew(reference, params) {
	XSCExpression.call(this);
	this.reference = reference;
	this.params = params;
}
XSCNew.prototype = Object.create(XSCExpression.prototype, {
	name: { value: "New", enumerable: true },
});
function XSCParams() {
	XSCNodes.call(this);
}
XSCParams.prototype = Object.create(XSCNodes.prototype, {
	name: { value: "Params", enumerable: true },
});

/* DELETE */

function XSCDelete(reference) {
	XSCExpression.call(this);
	this.reference = reference;
}
XSCDelete.prototype = Object.create(XSCExpression.prototype, {
	name: { value: "Delete", enumerable: true },
});

/* UNARY EXPRESSIONS */

function XSCUnaryExpression(right) {
	XSCExpression.call(this);
	this.right = right;
}
XSCUnaryExpression.prototype = Object.create(XSCExpression.prototype, {
});
function XSCTypeof(right) {
	XSCUnaryExpression.call(this, right);
}
XSCTypeof.prototype = Object.create(XSCUnaryExpression.prototype, {
	name: { value: "Typeof", enumerable: true },
});
function XSCVoid(right) {
	XSCUnaryExpression.call(this, right);
}
XSCVoid.prototype = Object.create(XSCUnaryExpression.prototype, {
	name: { value: "Void", enumerable: true },
});
function XSCSign(right) {
	XSCUnaryExpression.call(this, right);
}
XSCSign.prototype = Object.create(XSCUnaryExpression.prototype, {
});
function XSCMinus(right) {
	XSCSign.call(this, right);
}
XSCMinus.prototype = Object.create(XSCSign.prototype, {
	name: { value: "Minus", enumerable: true },
});
function XSCPlus(right) {
	XSCSign.call(this, right);
}
XSCPlus.prototype = Object.create(XSCSign.prototype, {
	name: { value: "Plus", enumerable: true },
});
function XSCBitNot(right) {
	XSCUnaryExpression.call(this, right);
}
XSCBitNot.prototype = Object.create(XSCUnaryExpression.prototype, {
	name: { value: "BitNot", enumerable: true },
});
function XSCNot(right) {
	XSCUnaryExpression.call(this, right);
}
XSCNot.prototype = Object.create(XSCUnaryExpression.prototype, {
	name: { value: "Not", enumerable: true },
});

/* PREFIX EXPRESSIONS */

function XSCPrefixExpression(right) {
	XSCUnaryExpression.call(this, right);
}
XSCPrefixExpression.prototype = Object.create(XSCUnaryExpression.prototype, {
});
function XSCPreDecrement(right) {
	XSCPrefixExpression.call(this, right);
}
XSCPreDecrement.prototype = Object.create(XSCPrefixExpression.prototype, {
	name: { value: "PreDecrement", enumerable: true },
});
function XSCPreIncrement(right) {
	XSCPrefixExpression.call(this, right);
}
XSCPreIncrement.prototype = Object.create(XSCPrefixExpression.prototype, {
	name: { value: "PreIncrement", enumerable: true },
});

/* POSTFIX EXPRESSIONS */

function XSCPostfixExpression(left) {
	XSCExpression.call(this);
	this.left = left;
}
XSCPostfixExpression.prototype = Object.create(XSCExpression.prototype, {
});
function XSCPostDecrement(left) {
	XSCPostfixExpression.call(this, left);
}
XSCPostDecrement.prototype = Object.create(XSCPostfixExpression.prototype, {
	name: { value: "PostDecrement", enumerable: true },
});
function XSCPostIncrement(left) {
	XSCPostfixExpression.call(this, left);
}
XSCPostIncrement.prototype = Object.create(XSCPostfixExpression.prototype, {
	name: { value: "PostIncrement", enumerable: true },
});

/* BINARY EXPRESSIONS */

function XSCBinaryExpression(left, right) {
	XSCExpression.call(this);
	this.left = left;
	this.right = right;
}
XSCBinaryExpression.prototype = Object.create(XSCExpression.prototype, {
});

function XSCAdd(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCAdd.prototype = Object.create(XSCBinaryExpression.prototype, {
	name: { value: "Add", enumerable: true },
});
function XSCSubtract(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCSubtract.prototype = Object.create(XSCBinaryExpression.prototype, {
	name: { value: "Subtract", enumerable: true },
});
function XSCMultiply(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCMultiply.prototype = Object.create(XSCBinaryExpression.prototype, {
	name: { value: "Multiply", enumerable: true },
});
function XSCDivide(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCDivide.prototype = Object.create(XSCBinaryExpression.prototype, {
	name: { value: "Divide", enumerable: true },
});
function XSCModulo(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCModulo.prototype = Object.create(XSCBinaryExpression.prototype, {
	name: { value: "Modulo", enumerable: true },
});

function XSCShiftOperation(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCShiftOperation.prototype = Object.create(XSCBinaryExpression.prototype, {
});
function XSCLeftShift(left, right) {
	XSCShiftOperation.call(this, left, right);
}
XSCLeftShift.prototype = Object.create(XSCShiftOperation.prototype, {
	name: { value: "LeftShift", enumerable: true },
});
function XSCSignedRightShift(left, right) {
	XSCShiftOperation.call(this, left, right);
}
XSCSignedRightShift.prototype = Object.create(XSCShiftOperation.prototype, {
	name: { value: "SignedRightShift", enumerable: true },
});
function XSCUnsignedRightShift(left, right) {
	XSCShiftOperation.call(this, left, right);
}
XSCUnsignedRightShift.prototype = Object.create(XSCShiftOperation.prototype, {
	name: { value: "UnsignedRightShift", enumerable: true },
});

function XSCBitOperation(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCBitOperation.prototype = Object.create(XSCBinaryExpression.prototype, {
});
function XSCBitAnd(left, right) {
	XSCBitOperation.call(this, left, right);
}
XSCBitAnd.prototype = Object.create(XSCBitOperation.prototype, {
	name: { value: "BitAnd", enumerable: true },
});
function XSCBitOr(left, right) {
	XSCBitOperation.call(this, left, right);
}
XSCBitOr.prototype = Object.create(XSCBitOperation.prototype, {
	name: { value: "BitOr", enumerable: true },
});
function XSCBitXor(left, right) {
	XSCBitOperation.call(this, left, right);
}
XSCBitXor.prototype = Object.create(XSCBitOperation.prototype, {
	name: { value: "BitXor", enumerable: true },
});

function XSCCompareExpression(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCCompareExpression.prototype = Object.create(XSCBinaryExpression.prototype, {
});
function XSCLess(left, right) {
	XSCCompareExpression.call(this, left, right);
}
XSCLess.prototype = Object.create(XSCCompareExpression.prototype, {
	name: { value: "Less", enumerable: true },
});
function XSCLessEqual(left, right) {
	XSCCompareExpression.call(this, left, right);
}
XSCLessEqual.prototype = Object.create(XSCCompareExpression.prototype, {
	name: { value: "LessEqual", enumerable: true },
});
function XSCMore(left, right) {
	XSCCompareExpression.call(this, left, right);
}
XSCMore.prototype = Object.create(XSCCompareExpression.prototype, {
	name: { value: "More", enumerable: true },
});
function XSCMoreEqual(left, right) {
	XSCCompareExpression.call(this, left, right);
}
XSCMoreEqual.prototype = Object.create(XSCCompareExpression.prototype, {
	name: { value: "MoreEqual", enumerable: true },
});
function XSCEqual(left, right) {
	XSCCompareExpression.call(this, left, right);
}
XSCEqual.prototype = Object.create(XSCCompareExpression.prototype, {
	name: { value: "Equal", enumerable: true },
});
function XSCNotEqual(left, right) {
	XSCCompareExpression.call(this, left, right);
}
XSCNotEqual.prototype = Object.create(XSCCompareExpression.prototype, {
	name: { value: "NotEqual", enumerable: true },
});
function XSCStrictEqual(left, right) {
	XSCCompareExpression.call(this, left, right);
}
XSCStrictEqual.prototype = Object.create(XSCCompareExpression.prototype, {
	name: { value: "StrictEqual", enumerable: true },
});
function XSCStrictNotEqual(left, right) {
	XSCCompareExpression.call(this, left, right);
}
XSCStrictNotEqual.prototype = Object.create(XSCCompareExpression.prototype, {
	name: { value: "StrictNotEqual", enumerable: true },
});

function XSCTestExpression(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCTestExpression.prototype = Object.create(XSCBinaryExpression.prototype, {
});
function XSCInstanceof(left, right) {
	XSCTestExpression.call(this, left, right);
}
XSCInstanceof.prototype = Object.create(XSCTestExpression.prototype, {
	name: { value: "Instanceof", enumerable: true },
});
function XSCIn(left, right) {
	XSCTestExpression.call(this, left, right);
}
XSCIn.prototype = Object.create(XSCTestExpression.prototype, {
	name: { value: "In", enumerable: true },
});

function XSCAnd(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCAnd.prototype = Object.create(XSCBinaryExpression.prototype, {
	name: { value: "And", enumerable: true },
});
function XSCOr(left, right) {
	XSCBinaryExpression.call(this, left, right);
}
XSCOr.prototype = Object.create(XSCBinaryExpression.prototype, {
	name: { value: "Or", enumerable: true },
});

function XSCQuestionMark(expression, thenExpression, elseExpression) {
	XSCExpression.call(this);
	this.expression = expression;
	this.thenExpression = thenExpression;
	this.elseExpression = elseExpression;
}
XSCQuestionMark.prototype = Object.create(XSCExpression.prototype, {
	name: { value: "QuestionMark", enumerable: true },
});

/* ASSIGNMENT EXPRESSIONS */

function XSCAssign(reference, value) {
	XSCExpression.call(this);
	this.reference = reference;
	this.value = value;
}
XSCAssign.prototype = Object.create(XSCExpression.prototype, {
	name: { value: "Assign", enumerable: true },
});
function XSCCompoundExpression(reference, value) {
	XSCAssign.call(this, reference, value);
}
XSCCompoundExpression.prototype = Object.create(XSCAssign.prototype, {
});
function XSCAddAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCAddAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "AddAssign", enumerable: true },
});
function XSCBitAndAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCBitAndAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "BitAndAssign", enumerable: true },
});
function XSCBitOrAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCBitOrAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "BitOrAssign", enumerable: true },
});
function XSCBitXorAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCBitXorAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "BitXorAssign", enumerable: true },
});
function XSCDivideAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCDivideAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "DivideAssign", enumerable: true },
});
function XSCLeftShiftAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCLeftShiftAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "LeftShiftAssign", enumerable: true },
});
function XSCModuloAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCModuloAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "ModuloAssign", enumerable: true },
});
function XSCMultiplyAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCMultiplyAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "MultiplyAssign", enumerable: true },
});
function XSCSignedRightShiftAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCSignedRightShiftAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "SignedRightShiftAssign", enumerable: true },
});
function XSCSubtractAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCSubtractAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "SubtractAssign", enumerable: true },
});
function XSCUnsignedRightShiftAssign(left, right) {
	XSCCompoundExpression.call(this, left, right);
}
XSCUnsignedRightShiftAssign.prototype = Object.create(XSCCompoundExpression.prototype, {
	name: { value: "UnsignedRightShiftAssign", enumerable: true },
});


/* STATEMENTS */

function XSCStatements() {
	XSCNodes.call(this);
}
XSCStatements.prototype = Object.create(XSCNodes.prototype, {
	name: { value: "Statements", enumerable: true },
});
function XSCStatement(expression) {
	XSCNode.call(this);
	this.expression = expression;
}
XSCStatement.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Statement", enumerable: true },
});
function XSCBreak(id) {
	XSCNode.call(this);
	this.id = id;
}
XSCBreak.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Break", enumerable: true },
});
function XSCContinue(id) {
	XSCNode.call(this);
	this.id = id;
}
XSCContinue.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Continue", enumerable: true },
});
function XSCDebugger() {
	XSCNode.call(this);
}
XSCDebugger.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Debugger", enumerable: true },
});
function XSCDo(statements, expression) {
	XSCNode.call(this);
	this.statements = statements;
	this.expression = expression;
}
XSCDo.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Do", enumerable: true },
});
function XSCFor(initialization, expression, iteration, statements) {
	XSCNode.call(this);
	this.initialization = initialization;
	this.expression = expression;
	this.iteration = iteration;
	this.statements = statements;
}
XSCFor.prototype = Object.create(XSCNode.prototype, {
	name: { value: "For", enumerable: true },
});
function XSCForIn(variable, expression, statements) {
	XSCNode.call(this);
	this.variable = variable;
	this.expression = expression;
	this.statements = statements;
}
XSCForIn.prototype = Object.create(XSCNode.prototype, {
	name: { value: "ForIn", enumerable: true },
});
function XSCIf(expression, statements, elseStatements) {
	XSCNode.call(this);
	this.expression = expression;
	this.statements = statements;
	this.elseStatements = elseStatements;
}
XSCIf.prototype = Object.create(XSCNode.prototype, {
	name: { value: "If", enumerable: true },
});
function XSCLabel(id, flag) {
	XSCNode.call(this);
	this.id = id;
}
XSCLabel.prototype = Object.create(XSCNode.prototype, {
	id: { value: "", writable: true },
	name: { value: "Label", writable: true },
});
function XSCReturn(expression) {
	XSCNode.call(this);
	this.expression = expression;
}
XSCReturn.prototype = Object.create(XSCNodes.prototype, {
	name: { value: "Return", enumerable: true },
});
function XSCSwitch(expression) {
	XSCNodes.call(this);
	this.expression = expression;
}
XSCSwitch.prototype = Object.create(XSCNodes.prototype, {
	name: { value: "Switch", enumerable: true },
});
function XSCCase(expression, statements) {
	XSCNode.call(this);
	this.expression = expression;
	this.statements = statements;
}
XSCCase.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Case", enumerable: true },
});
function XSCThrow(expression) {
	XSCNode.call(this);
	this.expression = expression;
}
XSCThrow.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Throw", enumerable: true },
});
function XSCTry(statements, exception, catchStatements, finallyStatements) {
	XSCNode.call(this);
	this.statements = statements;
	this.exception = exception;
	this.catchStatements = catchStatements;
	this.finallyStatements = finallyStatements;
}
XSCTry.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Try", enumerable: true },
});
function XSCWhile(expression, statements) {
	XSCNode.call(this);
	this.expression = expression;
	this.statements = statements;
}
XSCWhile.prototype = Object.create(XSCNode.prototype, {
	name: { value: "While", enumerable: true },
});
function XSCWith(expression, statements) {
	XSCNode.call(this);
	this.expression = expression;
	this.statements = statements;
}
XSCWith.prototype = Object.create(XSCNode.prototype, {
	name: { value: "With", enumerable: true },
});

/* FUNCTION */

function XSCFunction(id, args, vars, statements) {
	XSCNode.call(this);
	this.id = id;
	this.args = args;
	this.vars = vars;
	this.statements = statements;
}
XSCFunction.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Function", enumerable: true },
});

/* LOCALS */

function XSCLocals() {
	XSCNodes.call(this);
}
XSCLocals.prototype = Object.create(XSCNodes.prototype, {
	contains: {
		value: function(it) {
			if (this.items)
				return this.items.some(function(item) {
					return item.id == it.id;
				});
			return false;
		}
	},
});
function XSCLocal(id) {
	XSCNode.call(this);
	this.id = id;
}
XSCLocal.prototype = Object.create(XSCNode.prototype, {
});
function XSCArgs() {
	XSCLocals.call(this);
}
XSCArgs.prototype = Object.create(XSCLocals.prototype, {
	name: { value: "Args", enumerable: true },
});
function XSCArg(id) {
	XSCLocal.call(this, id);
}
XSCArg.prototype = Object.create(XSCLocal.prototype, {
	name: { value: "Arg", enumerable: true },
});
function XSCVars() {
	XSCLocals.call(this);
}
XSCVars.prototype = Object.create(XSCLocals.prototype, {
	name: { value: "Vars", enumerable: true }
});
function XSCVar(id) {
	XSCLocal.call(this, id);
}
XSCVar.prototype = Object.create(XSCLocal.prototype, {
	name: { value: "Var", enumerable: true },
});

function XSCArguments() {
	XSCNode.call(this);
}
XSCArguments.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Arguments", enumerable: true },
});

/* PROGRAM */

function XSCProgram(args, vars, statements, flag) {
	XSCNode.call(this);
	if (args) {
		this.args = args;
		this.id = null;
		this.name = "Function";
	}	
	this.vars = vars;
	this.statements = statements;
}
XSCProgram.prototype = Object.create(XSCNode.prototype, {
	name: { value: "Program", enumerable: true },
	compile: {
		value: function() {
			return JSON.stringify(this, undefined, 4);
		}
	},
});