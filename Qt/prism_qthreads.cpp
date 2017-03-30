#include "prism_qthreads.h"
#include "prism_progressbar.h"
#include "PRISM01.h"
#include "PRISM02.h"
#include "PRISM03.h"
#include "Multislice.h"
#include <iostream>

PotentialThread::PotentialThread(PRISMMainWindow *_parent, prism_progressbar *_progressbar) :
parent(_parent), progressbar(_progressbar){
    // construct the thread with a copy of the metadata so that any upstream changes don't mess with this calculation
    this->meta = *(parent->getMetadata());
}

void PotentialThread::run(){
    // create parameters
    PRISM::Parameters<PRISM_FLOAT_PRECISION> params(meta, progressbar);
    // calculate potential
    PRISM::PRISM01(params);
    // acquire the mutex so we can safely copy to the GUI copy of the potential
    QMutexLocker gatekeeper(&this->parent->potentialLock);
    // perform copy
    this->parent->potential = params.pot;
    // indicate that the potential is ready
    this->parent->potentialReady = true;
}

SMatrixThread::SMatrixThread(PRISMMainWindow *_parent, prism_progressbar *_progressbar) :
        parent(_parent), progressbar(_progressbar){
    // construct the thread with a copy of the metadata so that any upstream changes don't mess with this calculation
    this->meta = *(parent->getMetadata());
}

void SMatrixThread::run(){
    // create parameters
    PRISM::Parameters<PRISM_FLOAT_PRECISION> params(meta, progressbar);
    // calculate potential if it hasn't been already
    if (!this->parent->potentialReady){
	    QMutexLocker calculationLocker(&this->parent->calculationLock);
        // calculate potential
        PRISM::PRISM01(params);
        // acquire the mutex so we can safely copy to the GUI copy of the potential
        QMutexLocker gatekeeper(&this->parent->potentialLock);
        // perform copy
        this->parent->potential = params.pot;
        // indicate that the potential is ready
        this->parent->potentialReady = true;
    }
    std::cout << "calculating S-Matrix" << std::endl;
    // calculate S-Matrix
    PRISM::PRISM02(params);
    // acquire the mutex so we can safely copy to the GUI copy of the potential
    QMutexLocker gatekeeper(&this->parent->sMatrixLock);
    // perform copy
    this->parent->Scompact = params.Scompact;
    // indicate that the potential is ready
    this->parent->ScompactReady = true;
    std::cout << "copying S-Matrix" << std::endl;
    std::cout << "S-Matrix.at(0,0,0) = " << this->parent->Scompact.at(0,0,0) << std::endl;
}


FullPRISMCalcThread::FullPRISMCalcThread(PRISMMainWindow *_parent, prism_progressbar *_progressbar) :
parent(_parent), progressbar(_progressbar){
    // construct the thread with a copy of the metadata so that any upstream changes don't mess with this calculation
    this->meta = *(parent->getMetadata());
}


void FullPRISMCalcThread::run(){
    std::cout << "Full PRISM Calculation thread running" << std::endl;
    PRISM::Parameters<PRISM_FLOAT_PRECISION> params(meta, progressbar);
	QMutexLocker calculationLocker(&this->parent->calculationLock);
    PRISM::configure(meta);
//  //  PRISM::Parameters<PRISM_FLOAT_PRECISION> params = PRISM::execute_plan(meta);
    PRISM::PRISM01(params);
    std::cout <<"Potential Calculated" << std::endl;
    {
        QMutexLocker gatekeeper(&this->parent->potentialLock);
        this->parent->potential = params.pot;
        this->parent->potentialReady = true;
    }
    emit potentialCalculated();

    PRISM::PRISM02(params);
    // acquire the mutex so we can safely copy to the GUI copy of the potential
    {
        QMutexLocker gatekeeper(&this->parent->sMatrixLock);
        // perform copy
        this->parent->Scompact = params.Scompact;
        // indicate that the potential is ready
        this->parent->ScompactReady = true;
    }
    std::cout << "copying S-Matrix" << std::endl;
    emit ScompactCalculated();

    PRISM::PRISM03(params);
    {
        QMutexLocker gatekeeper(&this->parent->outputLock);
        this->parent->output = params.stack;
	    this->parent->detectorAngles = params.detectorAngles;
	    for (auto& a:this->parent->detectorAngles) a*=1000; // convert to mrads
        this->parent->outputReady = true;
        size_t lower = 13;
        size_t upper = 18;
        PRISM::Array2D<PRISM_FLOAT_PRECISION> prism_image;
        prism_image = PRISM::zeros_ND<2, PRISM_FLOAT_PRECISION>({{params.stack.get_diml(), params.stack.get_dimk()}});
        for (auto y = 0; y < params.stack.get_diml(); ++y){
            for (auto x = 0; x < params.stack.get_dimk(); ++x){
                for (auto b = lower; b < upper; ++b){
                    prism_image.at(y,x) += params.stack.at(y,x,b,0);
                }
            }
        }

        prism_image.toMRC_f(params.meta.filename_output.c_str());
    }
    emit outputCalculated();

    std::cout << "Calculation complete" << std::endl;
    std::cout<<"after copy this->parent->output.at(0,0,0) = " << this->parent->output.at(0,0,0) << std::endl;
}


FullMultisliceCalcThread::FullMultisliceCalcThread(PRISMMainWindow *_parent, prism_progressbar *_progressbar) :
        parent(_parent), progressbar(_progressbar){
    // construct the thread with a copy of the metadata so that any upstream changes don't mess with this calculation
    this->meta = *(parent->getMetadata());
}


void FullMultisliceCalcThread::run(){
    std::cout << "Full Multislice Calculation thread running" << std::endl;
    PRISM::Parameters<PRISM_FLOAT_PRECISION> params(meta, progressbar);
	QMutexLocker calculationLocker(&this->parent->calculationLock);
    PRISM::configure(meta);
    PRISM::PRISM01(params);
    std::cout <<"Potential Calculated" << std::endl;
    {
        QMutexLocker gatekeeper(&this->parent->potentialLock);
        this->parent->potential = params.pot;
        this->parent->potentialReady = true;
    }
    emit potentialCalculated();

    PRISM::Multislice(params);
    {
        QMutexLocker gatekeeper(&this->parent->outputLock);
        this->parent->output = params.stack;
	    this->parent->detectorAngles = params.detectorAngles;
	    for (auto& a:this->parent->detectorAngles) a*=1000; // convert to mrads
        this->parent->outputReady = true;
        size_t lower = 13;
        size_t upper = 18;
        PRISM::Array2D<PRISM_FLOAT_PRECISION> prism_image;
        prism_image = PRISM::zeros_ND<2, PRISM_FLOAT_PRECISION>({{params.stack.get_diml(), params.stack.get_dimk()}});
        for (auto y = 0; y < params.stack.get_diml(); ++y){
            for (auto x = 0; x < params.stack.get_dimk(); ++x){
                for (auto b = lower; b < upper; ++b){
                    prism_image.at(y,x) += params.stack.at(y,x,b,0);
                }
            }
        }

        prism_image.toMRC_f(params.meta.filename_output.c_str());
    }
    emit outputCalculated();


    std::cout << "Calculation complete" << std::endl;
    std::cout<<"after copy this->parent->output.at(0,0,0) = " << this->parent->output.at(0,0,0) << std::endl;

}

PotentialThread::~PotentialThread(){}
SMatrixThread::~SMatrixThread(){}
FullPRISMCalcThread::~FullPRISMCalcThread(){}
FullMultisliceCalcThread::~FullMultisliceCalcThread(){}
