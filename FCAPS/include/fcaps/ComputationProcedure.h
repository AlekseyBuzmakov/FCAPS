// Initial software, Aleksey Buzmakov, Copyright (c) National Research University Higher School of Economics, GPL v2 license, 2018, v0.8

// Author: Aleksey Buzmakov
// Description: Interface of algorithms building FCA lattices.

#ifndef COMPUTATIONPROCEDURE_H_
#define COMPUTATIONPROCEDURE_H_

#include <common.h>

#include <fcaps/BasicTypes.h>

#include <string>

////////////////////////////////////////////////////////////////////////

interface IComputationCallback;

////////////////////////////////////////////////////////////////////

const char ComputationProcedureModuleType[] = "ComputationProcedureModules";

interface IComputationProcedure : public virtual IObject {
	// Callback for progress reporting.
	virtual void SetCallback( const IComputationCallback * cb ) = 0;

	// The method runs the computations
	virtual void Run() = 0;

	// Saving a result of the computation procedure to file(s) with the base name given by basePath
	virtual void SaveResult( const std::string& basePath ) = 0;
};

////////////////////////////////////////////////////////////////////////

interface IComputationCallback : public virtual IObject {
	// Reports progress of the method
	virtual void ReportProgress( const double& p, const std::string& info ) const = 0;
	virtual void ReportNextStage( const std::string& stageName ) const = 0;
	// Indicates found warnings
	virtual void Warning(const std::string& warning) const = 0;
	// Asks if the computations has been interrupted
	virtual bool IsInterrupted() const = 0;
};

////////////////////////////////////////////////////////////////////////

#endif // COMPUTATIONPROCEDURE_H_
