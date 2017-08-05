An attempt to pass MATLAB function handles to Python, possibly leveraging MATLAB's Python bridge.

Possibly use `ctypes` for erasure from MATLAB back to Python, like:
https://github.com/dgorissen/pymatopt

# Notes

    To convert MATLAB to Python:

        >> ML
        >> py.pass_through({arg1, arg2, ...})
        py.tuple(...)
        >> py_ml_sin = py.ml_wrap.wrap(@sin)
        >> py.call(py_ml_sin, 0)
        0

    To convert Python to MATLAB

        >> ML
        >> mex_args = mex_opaque({arg1, arg2, ...})


Example:

    Direct

        Python
            def call_func(f, arg1, arg2):
                f(arg1, arg2)

        MATLAB
            function my_func(arg1, arg2)
                disp(arg1)
                disp(arg2)
            end

            py_call_func_direct(@my_func, struct('a', 1), eye(3))

        Breakdown:
            py_call_func_direct
                Direct call.
                In MATLAB
                    Will intercept all args, wrap into opaque argument set ("rhs")
                    Pass to Python with opaque args
                In Python
                    Will invoke ctypes to pass func handle and arguments directly through

    Indirect
        # https://stackoverflow.com/questions/15011674/is-it-possible-to-dereference-variable-ids
        # https://stackoverflow.com/questions/3245859/back-casting-a-ctypes-py-object-in-a-callback
        # Concerns: https://stackoverflow.com/questions/18660433/matlab-mex-file-with-mexcallmatlab-is-almost-300-times-slower-than-the-correspon

        Python - mx_py.py
            def call_func(f, x, str):
                call_matlab(f, 1, 2 * x, str + " world")

            from ctypes import *
            mx_array_t = c_void_p  # mxArray*
            mx_array_t_p = c_void_p  # mxArray**

            # int (mxArray* func_handle, void_p) # int nlhs, mxArray* plhs[], int nrhs, mxArray* prhs[])
            c_mx_feval_py_raw_t = CFUNCTYPE(py_object, mx_array_t, c_int, mx_array_t_p, c_int, mx_array_t_p)

            # void* (void*) - but use ctypes to extract void* from py_object
            c_raw_to_py_t = ctypes.CFUNCTYPE(py_object, c_void_p)  # Use py_object so ctypes can cast to void*
            c_py_to_raw_t = ctypes.CFUNCTYPE(c_void_p, py_object)

            # py - Python
            # py_raw - Raw Python points (py_object, c_void_p)
            # ml, mx - MATLAB
            # mx_raw - void* representing mxArray* (mx_array_t, c_void_p)
            # mex - MEX function

            funcs = {}

            def declare_c_func_ptrs(funcs_in):
                global funcs
                # Effectively re-interperet casts.
                funcs['c_raw_to_py'] = \
                    c_raw_to_py_t(funcs_in['c_raw_to_py'])
                funcs['c_py_to_raw'] = \
                    c_py_to_raw_t(funcs_in['c_py_to_raw'])
                # Calling MEX through type erasure.
                funcs['c_mx_feval_py_raw'] = \
                    c_mx_feval_py_raw_t(funcs_in['c_mx_feval_py_raw'])

            def py_raw_to_py(py_raw):
                return funcs['c_raw_to_py'](py_raw)

            def py_to_py_raw(obj):
                return funcs['c_py_to_raw'](obj)

            def call_matlab(mx_raw_func_handle, nargout, *py_in):
                nargin = len(py_in)
                # Just do a py.list, for MATLAB to convert to a cell arrays.
                py_raw_in = c_py_to_raw(py_in)
                py_raw_out = funcs['c_mx_feval_py_raw'](mx_raw_func_handle, nargout, py_raw_in)
                py_out = c_raw_to_py(nargout, py_raw_out)
                return py_out

                # Is there a way to use mexCallMATLAB to be invoked on Python args?
                # Increase reference counting???
        MATLAB - PyProxyMex
            function init()
                py.mx_py.declare_c_func_ptrs(mex_py_proxy('get_c_func_ptrs'));
                py.mx_py.call_matlab(mex_py_proxy('mx_to_mx_raw', @sin), 1, 0.)
            end

            function py_raw_out = mx_feval_py_raw(f, nout, py_raw_in)
                % feval_py 
                % Input: void* representing a Python list containing all input
                % arguments to be converted to MATLAB.
                % Output: void* representing a Python list containing all output.
                py_in = py.py_raw_to_py(py_raw_in)
                mx_in = PyProxy.fromPy(py_in);  % Add depth option?
                mx_out = cell(1, nout);
                [mx_out{:}] = feval(f, mx_in{:});
                py_out = PyProxy.toPy(mx_out);
                py_raw_out = uint64(py.py_to_py_raw(py_out));
            end
        C - mex_py_proxy.cpp
            void* void_p_pass_thru(void* in) {
                // Leverage Python's ctypes marhsalling of py_object is a (hopefully)
                // robust mechanism to pass stuff around.
                return out;
            }
            void* c_mx_feval_py_raw(void* mx_raw_handle, int nout, void* py_raw_in) {
                mxArray* mx_handle = mx_raw_handle;
                mxArray* mx_nout = mxCreateNumericMatrix(1, 1, nout);
                mxArray* mx_py_raw_in = mxCreateNumericMatrix(1, 1, py_raw_in);

                mxArray* mx_in[] = {mx_raw_handle, mx_nout, mx_py_raw_in};
                mxArray* mx_out[] = {NULL};
                mxArray* mx_handle = static_cast<mxArray*>(mx_raw_handle);
                mexCallMATLAB("mx_feval_py_raw", ...)
                void* py_raw_out = *static_cast<void**>(mxGetData(mx_out[0]));

                mxFree(mx_nout);
                mxFree(mx_out);

                return py_raw_out;
            }


TODO: Test passing opaque function pointers from `pybind11`.
    Done.