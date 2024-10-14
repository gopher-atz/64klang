using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MultiParse
{
    /// <summary>
    /// An interface describing an assignable object
    /// </summary>
    public interface IMPAssignable
    {
        void Assign(object value);
    }

    /// <summary>
    /// An interface describing a gettable object
    /// </summary>
    public interface IMPGettable
    {
        object Get();
    }

    /// <summary>
    /// A class describing a data type
    /// </summary>
    public abstract class MPDataType
    {

        /// <summary>
        /// Returns -1 if the operator doesn't match. Else it returns the length of the matched string.
        /// </summary>
        /// <param name="expression">The expression</param>
        /// <param name="previousToken">The previous expression</param>
        /// <returns></returns>
        public abstract int Match(string expression, object previousToken, out object convertedToken);

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
    }
}
