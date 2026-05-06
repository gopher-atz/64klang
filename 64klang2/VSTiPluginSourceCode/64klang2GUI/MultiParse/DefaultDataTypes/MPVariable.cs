using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace MultiParse.Default
{

    /// <summary>
    /// A instance of a variable
    /// </summary>
    public class MPVariableInstance : IMPAssignable, IMPGettable
    {
        // The value of the instance
        private object value;
        private string name;

        /// <summary>
        /// Constructor
        /// </summary>
        public MPVariableInstance(string _name)
        {
            value = null;
            name = _name;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="value"></param>
        public MPVariableInstance(object value, string _name)
        {
            this.value = value;
            this.name = _name;
        }

        /// <summary>
        /// Assign an object to the instance
        /// </summary>
        /// <param name="value"></param>
        public void Assign(object value)
        {
            this.value = value;
        }

        /// <summary>
        /// Get the value of the instance
        /// </summary>
        /// <returns></returns>
        public object Get()
        {
            return this.value;
        }

        public override string ToString()
        {
            return name;
        }
    }

    /// <summary>
    /// Can generate
    /// </summary>
    public class MPVariable : MPDataType
    {
        /// <summary>
        /// True if unknown variables are added dynamically
        /// </summary>
        public bool AutoGenerate;

        /// <summary>
        /// A list of variables
        /// </summary>
        public Dictionary<string, MPVariableInstance> Variables { get { return variables; } }
        private Dictionary<string, MPVariableInstance> variables;

        /// <summary>
        /// Constructor
        /// </summary>
        public MPVariable()
            : base()
        {
            this.AutoGenerate = true;
            this.variables = new Dictionary<string, MPVariableInstance>();
        }

        /// <summary>
        /// Match a generic variable in the expression
        /// </summary>
        /// <param name="expression"></param>
        /// <param name="previousToken"></param>
        /// <param name="convertedToken"></param>
        /// <returns></returns>
        public override int Match(string expression, object previousToken, out object convertedToken)
        {
            Match m = Regex.Match(expression, @"^[a-zA-Z_]\w*(?![\w\.\(\[])");
            if (m.Success)
            {
                // Handle non-existing variables
                if (!variables.ContainsKey(m.Value))
                {
                    if (AutoGenerate)
                        variables.Add(m.Value, new MPVariableInstance((double)0.0, m.Value));
                    else
                    {
                        convertedToken = null;
                        return -1;
                    }
                }

                // Return the variable
                convertedToken = variables[m.Value];
                return m.Length;
            }

            // Not recognized
            convertedToken = null;
            return -1;
        }
    }
}
