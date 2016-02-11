#pragma once

#include "NonlinSolver.h"
#include "TimeDiscretization.h"
#include "MatrixTranslator.h"

#include <memory>


template<NonlinearSolverTag NLTag>
class TimeDiscretizedODESystem
        : public INonlinearSystem<NLTag>
        , public InternalMatrixStorage
{
public:
    virtual ITimeDiscretization& getTimeDiscretization() = 0;
};

template<NonlinearSolverTag NLTag>
class TimeDiscretizedFirstOrderImplicitQuasilinearODESystem;

template<>
class TimeDiscretizedFirstOrderImplicitQuasilinearODESystem<NonlinearSolverTag::Newton> final
        : public TimeDiscretizedODESystem<NonlinearSolverTag::Newton>
{
public:
    explicit
    TimeDiscretizedFirstOrderImplicitQuasilinearODESystem(
            FirstOrderImplicitQuasilinearODESystem<NonlinearSolverTag::Newton>& ode,
            ITimeDiscretization& time_discretization,
            MatrixTranslator<EquationTag::FirstOrderImplicitQuasilinearODESystem>& mat_trans)
        : _ode(ode)
        , _time_disc(time_discretization)
        , _mat_trans(mat_trans)
        , _Jac(ode.getMatrixSize(), ode.getMatrixSize())
        , _M(_Jac)
        , _K(_Jac)
        , _b(ode.getMatrixSize())
    {}

    /// begin INonlinearSystemNewton

    void assembleResidualNewton(const Vector &x_new_timestep) override
    {
        auto const  t      = _time_disc.getCurrentTime();
        auto const& x_curr = _time_disc.getCurrentX(x_new_timestep);

        _ode.assemble(t, x_curr, _M, _K, _b);
    }

    void assembleJacobian(const Vector &x_new_timestep) override
    {
        auto const  t        = _time_disc.getCurrentTime();
        auto const& x_curr   = _time_disc.getCurrentX(x_new_timestep);
        auto const  dxdot_dx = _time_disc.getCurrentXWeight();

        _ode.assembleJacobian(t, x_curr, dxdot_dx, _time_disc.getDxDx(), _Jac);
    }

    Vector getResidual(Vector const& x_new_timestep) override
    {
        return _mat_trans.getResidual(_M, _K, _b, x_new_timestep);
    }

    Matrix getJacobian() override
    {
        return _mat_trans.getJacobian(_Jac);
    }

    bool isLinear() const override
    {
        return _time_disc.isLinearTimeDisc() || _ode.isLinear();
    }

    /// end INonlinearSystemNewton

    ITimeDiscretization& getTimeDiscretization() override {
        return _time_disc;
    }

private:
    // from IParabolicEquation
    virtual void pushMatrices() const override
    {
        _mat_trans.pushMatrices(_M, _K, _b);
    }


    FirstOrderImplicitQuasilinearODESystem<NonlinearSolverTag::Newton>& _ode;
    ITimeDiscretization& _time_disc;
    MatrixTranslator<EquationTag::FirstOrderImplicitQuasilinearODESystem>& _mat_trans;

    Matrix _Jac;
    Matrix _M;
    Matrix _K;
    Vector _b;
};

template<>
class TimeDiscretizedFirstOrderImplicitQuasilinearODESystem<NonlinearSolverTag::Picard> final
        : public TimeDiscretizedODESystem<NonlinearSolverTag::Picard>
{
public:
    explicit
    TimeDiscretizedFirstOrderImplicitQuasilinearODESystem(
            FirstOrderImplicitQuasilinearODESystem<NonlinearSolverTag::Picard>& ode,
            ITimeDiscretization& time_discretization,
            MatrixTranslator<EquationTag::FirstOrderImplicitQuasilinearODESystem>& mat_trans)
        : _ode(ode)
        , _time_disc(time_discretization)
        , _mat_trans(mat_trans)
        , _M(ode.getMatrixSize(), ode.getMatrixSize())
        , _K(_M)
        , _b(ode.getMatrixSize())
    {}

    /// begin INonlinearSystemNewton

    void assembleMatricesPicard(const Vector &x_new_timestep) override
    {
        auto const  t      = _time_disc.getCurrentTime();
        auto const& x_curr = _time_disc.getCurrentX(x_new_timestep);

        _ode.assemble(t, x_curr, _M, _K, _b);
    }

    Matrix getA() override
    {
        return _mat_trans.getA(_M, _K);
    }

    Vector getRhs() override
    {
        return _mat_trans.getRhs(_M, _K, _b);
    }

    bool isLinear() const override
    {
        return _time_disc.isLinearTimeDisc() || _ode.isLinear();
    }

    /// end INonlinearSystemPicard

    ITimeDiscretization& getTimeDiscretization() override {
        return _time_disc;
    }

private:
    // from IParabolicEquation
    virtual void pushMatrices() const override
    {
        _mat_trans.pushMatrices(_M, _K, _b);
    }


    FirstOrderImplicitQuasilinearODESystem<NonlinearSolverTag::Picard>& _ode;
    ITimeDiscretization& _time_disc;
    MatrixTranslator<EquationTag::FirstOrderImplicitQuasilinearODESystem>& _mat_trans;

    Matrix _M;
    Matrix _K;
    Vector _b;
};
