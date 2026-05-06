using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MultiParse
{
    /// <summary>
    /// A class representing an operator
    /// </summary>
    public abstract class MPOperator
    {
        // C# default precedences
        protected static int PrecedenceAssignment = 0; // right-associative by default
        protected static int PrecedenceConditional = 1; // right-associative by default
        protected static int PrecedenceConditionalOr = 2;
        protected static int PrecedenceConditionalAnd = 3;
        protected static int PrecedenceLogicalOr = 4;
        protected static int PrecedenceLogicalXOr = 5;
        protected static int PrecedenceLogicalAnd = 6;
        protected static int PrecedenceEquality = 7;
        protected static int PrecedenceRelational = 8;
        protected static int PrecedenceShift = 9;
        protected static int PrecedenceAdditive = 10;
        protected static int PrecedenceMultiplicative = 11;
        protected static int PrecedenceUnary = 12;
        protected static int PrecedencePrimary = 13;

        // Private variables
        protected int precedence;
        protected bool leftAssociative;

        /// <summary>
        /// Contains the key for the operator
        /// This is used to check for duplicate operator codes
        /// </summary>
        protected string key;
        public string Key { get { return key; } }

        /// <summary>
        /// Constructor
        /// </summary>
        public MPOperator(string key, int precedence, bool leftAssociative)
        {
            this.key = key;
            this.precedence = precedence;
            this.leftAssociative = leftAssociative;
        }

        /// <summary>
        /// Gets an integer representing the precedence of the operator
        /// </summary>
        public int Precedence { get { return precedence; } }

        /// <summary>
        /// Gets a bool signifying whether or not the operator is left-associative
        /// </summary>
        public bool LeftAssociative { get { return leftAssociative; } }

        /// <summary>
        /// Returns -1 if the operator doesn't match. Else it returns the length of the matched string.
        /// </summary>
        /// <param name="expression">The expression</param>
        /// <param name="previousToken">The previous expression</param>
        /// <returns></returns>
        public abstract int Match(string expression, object previousToken);

        /// <summary>
        /// Returns an object while altering the stack
        /// </summary>
        /// <param name="output"></param>
        /// <returns></returns>
        public abstract void Execute(Stack<object> output);

        /// <summary>
        /// Pop an operand of an operator from the stack
        /// If the object is a IMPGettable, the object is the resolved object
        /// </summary>
        /// <param name="output"></param>
        /// <returns></returns>
        protected object PopOrGet(Stack<object> output)
        {
            object top = output.Pop();
            if (top is IMPGettable)
                return ((IMPGettable)top).Get();
            return top;
        }

        /// <summary>
        /// Check whether or not it is possible to have a binary operator
        /// </summary>
        /// <param name="previousToken"></param>
        /// <returns></returns>
        protected bool IsUnary(object previousToken)
        {
            if (previousToken == null)
                return true;
            if (previousToken is MPOperator)
                return true;
            if (previousToken is BracketOpen)
                return true;
            if (previousToken is FunctionSeparator)
                return true;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a bool from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out bool result)
        {
            if (o is Boolean)
            {
                result = (Boolean)o;
                return true;
            }
            result = false;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a byte from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out byte result)
        {
            if (o is Byte)
            {
                result = (Byte)o;
                return true;
            }

            result = 0;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a char from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out char result)
        {
            if (o is Char)
            {
                result = (Char)o;
                return true;
            }
            result = '\0';
            return false;
        }

        /// <summary>
        /// Implicit conversion to a decimal from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out decimal result)
        {
            TypeCode tc = Type.GetTypeCode(o.GetType());
            switch (tc)
            {
                case TypeCode.Byte: result = (Byte)o; return true;
                case TypeCode.Char: result = (Char)o; return true;
                case TypeCode.Decimal: result = (Decimal)o; return true;
                case TypeCode.Int16: result = (Int16)o; return true;
                case TypeCode.Int32: result = (Int32)o; return true;
                case TypeCode.Int64: result = (Int64)o; return true;
                case TypeCode.SByte: result = (SByte)o; return true;
                case TypeCode.UInt16: result = (UInt16)o; return true;
                case TypeCode.UInt32: result = (UInt32)o; return true;
                case TypeCode.UInt64: result = (UInt64)o; return true;
            }
            result = 0;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a double from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out double result)
        {
            TypeCode tc = Type.GetTypeCode(o.GetType());
            switch (tc)
            {
                case TypeCode.Byte: result = (Byte)o; return true;
                case TypeCode.Char: result = (Char)o; return true;
                case TypeCode.Double: result = (Double)o; return true;
                case TypeCode.Int16: result = (Int16)o; return true;
                case TypeCode.Int32: result = (Int32)o; return true;
                case TypeCode.Int64: result = (Int64)o; return true;
                case TypeCode.SByte: result = (SByte)o; return true;
                case TypeCode.Single: result = (Single)o; return true;
                case TypeCode.UInt16: result = (UInt16)o; return true;
                case TypeCode.UInt32: result = (UInt32)o; return true;
                case TypeCode.UInt64: result = (UInt64)o; return true;
            }
            result = 0.0;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a short from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out short result)
        {
            TypeCode tc = Type.GetTypeCode(o.GetType());
            switch (tc)
            {
                case TypeCode.Byte: result = (Byte)o; return true;
                case TypeCode.Int16: result = (Int16)o; return true;
                case TypeCode.SByte: result = (SByte)o; return true;
            }
            result = 0;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a int from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out int result)
        {
            TypeCode tc = Type.GetTypeCode(o.GetType());
            switch (tc)
            {
                case TypeCode.Byte: result = (Byte)o; return true;
                case TypeCode.Char: result = (Char)o; return true;
                case TypeCode.Int16: result = (Int16)o; return true;
                case TypeCode.Int32: result = (Int32)o; return true;
                case TypeCode.SByte: result = (SByte)o; return true;
                case TypeCode.UInt16: result = (UInt16)o; return true;
            }
            result = 0;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a long from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out long result)
        {
            TypeCode tc = Type.GetTypeCode(o.GetType());
            switch (tc)
            {
                case TypeCode.Byte: result = (Byte)o; return true;
                case TypeCode.Char: result = (Char)o; return true;
                case TypeCode.Int16: result = (Int16)o; return true;
                case TypeCode.Int32: result = (Int32)o; return true;
                case TypeCode.Int64: result = (Int64)o; return true;
                case TypeCode.SByte: result = (SByte)o; return true;
                case TypeCode.UInt16: result = (UInt16)o; return true;
                case TypeCode.UInt32: result = (UInt32)o; return true;
            }
            result = 0;
            return false;
        }

        /// <summary>
        /// Implicit conversion to an sbyte from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out sbyte result)
        {
            if (o is SByte)
            {
                result = (SByte)o;
                return true;
            }

            result = 0;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a float from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out float result)
        {
            TypeCode tc = Type.GetTypeCode(o.GetType());
            switch (tc)
            {
                case TypeCode.Byte: result = (Byte)o; return true;
                case TypeCode.Char: result = (Char)o; return true;
                case TypeCode.Int16: result = (Int16)o; return true;
                case TypeCode.Int32: result = (Int32)o; return true;
                case TypeCode.Int64: result = (Int64)o; return true;
                case TypeCode.SByte: result = (SByte)o; return true;
                case TypeCode.Single: result = (Single)o; return true;
                case TypeCode.UInt16: result = (UInt16)o; return true;
                case TypeCode.UInt32: result = (UInt32)o; return true;
                case TypeCode.UInt64: result = (UInt64)o; return true;
            }
            result = 0f;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a ushort from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out ushort result)
        {
            TypeCode tc = Type.GetTypeCode(o.GetType());
            switch (tc)
            {
                case TypeCode.Byte: result = (Byte)o; return true;
                case TypeCode.Char: result = (Char)o; return true;
                case TypeCode.UInt16: result = (UInt16)o; return true;
            }
            result = 0;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a uint from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out uint result)
        {
            TypeCode tc = Type.GetTypeCode(o.GetType());
            switch (tc)
            {
                case TypeCode.Byte: result = (Byte)o; return true;
                case TypeCode.Char: result = (Char)o; return true;
                case TypeCode.UInt16: result = (UInt16)o; return true;
                case TypeCode.UInt32: result = (UInt32)o; return true;
            }
            result = 0;
            return false;
        }

        /// <summary>
        /// Implicit conversion to a ulong from all main types. Returns true if the object can be casted.
        /// </summary>
        /// <param name="o"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        protected static bool CastImplicit(object o, out ulong result)
        {
            TypeCode tc = Type.GetTypeCode(o.GetType());
            switch (tc)
            {
                case TypeCode.Byte: result = (Byte)o; return true;
                case TypeCode.Char: result = (Char)o; return true;
                case TypeCode.UInt16: result = (UInt16)o; return true;
                case TypeCode.UInt32: result = (UInt32)o; return true;
                case TypeCode.UInt64: result = (UInt64)o; return true;
            }
            result = 0;
            return false;
        }
    }
}
