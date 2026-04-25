#ifndef FORCEFUNCTION_H
#define FORCEFUNCTION_H

#include <QString>
#include <QJSEngine>
#include <QJSValue>

// Holds an arbitrary force expression F(t) and a direction angle.
// The expression is evaluated as JavaScript, with these variables available:
//   t   – time (s)
//   w   – angular frequency (rad/s), user-supplied
//   pi  – π
//   e   – Euler's number
// All standard JS Math functions work: sin, cos, exp, abs, sqrt, pow, etc.
//   e.g.  "10*sin(w*t)"   "3*exp(-5*t)"   "5*(t<2 ? t : 2)"
class ForceFunction
{
public:
    ForceFunction() = default;

    ForceFunction(const QString &expression,
                  double angleDeg,
                  double omega)
        : m_expr(expression), m_angleDeg(angleDeg), m_omega(omega)
    {}

    // Evaluate the expression at time t. Returns 0 on error.
    double evaluate(double t) const
    {
        if (m_expr.isEmpty()) return 0.0;

        QJSEngine eng;
        // Inject constants and parameters
        eng.globalObject().setProperty("t",  t);
        eng.globalObject().setProperty("w",  m_omega);
        eng.globalObject().setProperty("pi", M_PI);
        eng.globalObject().setProperty("e",  M_E);

        // Map Math.sin → sin etc. so bare names work
        eng.evaluate(
            "var sin=Math.sin, cos=Math.cos, tan=Math.tan,"
            "    exp=Math.exp, log=Math.log, sqrt=Math.sqrt,"
            "    abs=Math.abs, pow=Math.pow, floor=Math.floor,"
            "    ceil=Math.ceil, round=Math.round, sign=Math.sign;"
            );

        QJSValue result = eng.evaluate(m_expr);
        if (result.isError() || !result.isNumber()) return 0.0;
        return result.toNumber();
    }

    // Validate expression without side effects. Returns empty string on success,
    // or an error message on failure.
    static QString validate(const QString &expr, double omega)
    {
        QJSEngine eng;
        eng.globalObject().setProperty("t",  0.0);
        eng.globalObject().setProperty("w",  omega);
        eng.globalObject().setProperty("pi", M_PI);
        eng.globalObject().setProperty("e",  M_E);
        eng.evaluate(
            "var sin=Math.sin, cos=Math.cos, tan=Math.tan,"
            "    exp=Math.exp, log=Math.log, sqrt=Math.sqrt,"
            "    abs=Math.abs, pow=Math.pow, floor=Math.floor,"
            "    ceil=Math.ceil, round=Math.round, sign=Math.sign;"
            );
        QJSValue r = eng.evaluate(expr);
        if (r.isError()) return r.toString();
        if (!r.isNumber()) return "Expression did not return a number.";
        return {};
    }

    bool    isEmpty()    const { return m_expr.isEmpty(); }
    QString expression() const { return m_expr; }
    double  angleDeg()   const { return m_angleDeg; }
    double  omega()      const { return m_omega; }

private:
    QString m_expr;
    double  m_angleDeg = 0.0;
    double  m_omega    = 1.0;
};

#endif