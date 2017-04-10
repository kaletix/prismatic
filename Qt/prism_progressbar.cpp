#include "prism_progressbar.h"
#include "ui_prism_progressbar.h"
#include <algorithm>
prism_progressbar::prism_progressbar(PRISMMainWindow *_parent) :
    parent(_parent),
    ui(new Ui::prism_progressbar),
    potentialCurrentSlice(0),
    potentialTotalSlices(0),
    SMatrixCurrentBeam(0),
    SMatrixTotalBeams(0),
    currentProbe(0),
    totalProbes(0)
{
    ui->setupUi(this);
    ui->progressBar->setValue(0);
    connect(this, SIGNAL(updateDescriptionMessage(QString)), this, SLOT(updateDescription(QString)));
    connect(this, SIGNAL(updateCalcStatus(QString)), this, SLOT(updateCalcStatusMessage(QString)));
	connect(this, SIGNAL(updateProgressBar(int)), ui->progressBar, SLOT(setValue(int)));

}

void prism_progressbar::setStepPotential(){
    ui->lbl_Description->setText(QString("Computing Projected Potential Slices"));
}

//void prism_progressbar::setAlgorithmPRISM(){
//    ui->lbl_calcStatus->setText("Computing Projected Potential Slices");
//}

void prism_progressbar::update_calculatingPotential(long current, long total){
    potentialCurrentSlice = std::max(potentialCurrentSlice, current);
    ui->lbl_calcStatus->setText(QString("Slice ") +
                                 QString::number(current) +
                                 QString("/") +
                                 QString::number(total));
}

void prism_progressbar::updateDescription(const QString str){
    ui->lbl_Description->setText(str);
}
//void prism_progressbar::setText(const QString str){
//    ui->lbl_Description->setText(str);
//}
void prism_progressbar::setProgress(int val){
	emit updateProgressBar(val);
}
void prism_progressbar::updateCalcStatusMessage(const QString str){
    ui->lbl_calcStatus->setText(str);
}
void prism_progressbar::signalCalcStatusMessage(const QString str){
    emit updateCalcStatus(str);
}
void prism_progressbar::signalDescriptionMessage(const QString str){
    emit updateDescriptionMessage(str);
}

void prism_progressbar::signalPotentialUpdate(const long current, const long total){
//    std::lock_guard<std::mutex> gatekeeper(this->parent->potentialLock);
	potentialCurrentSlice = std::max(potentialCurrentSlice, current);
	emit updateCalcStatus(QString("Slice ") +
	                      QString::number(potentialCurrentSlice + 1) +
	                      QString("/") +
	                      QString::number(total));
	emit updateProgressBar(100*(current+1)/total);
}
void prism_progressbar::signalScompactUpdate(const long current, const long total){
//    std::lock_guard<std::mutex> gatekeeper(this->parent->sMatrixLock);
    SMatrixCurrentBeam = std::max(SMatrixCurrentBeam, current);
//    std::cout << "SMatrixCurrentBeam + 1 = " << SMatrixCurrentBeam + 1 << std::endl;
    emit updateCalcStatus(QString("Slice ") +
                          QString::number(SMatrixCurrentBeam + 1) +
                          QString("/") +
                          QString::number(total));
    emit updateProgressBar(100*(SMatrixCurrentBeam+1)/total);
}
void prism_progressbar::signalOutputUpdate(const long current, const long total){
//    std::lock_guard<std::mutex> gatekeeper(this->parent->outputLock);
    currentProbe = std::max(currentProbe, current);
//    std::cout << "SMatrixCurrentBeam + 1 = " << SMatrixCurrentBeam + 1 << std::endl;
    emit updateCalcStatus(QString("Probe Position ") +
                          QString::number(currentProbe + 1) +
                          QString("/") +
                          QString::number(total));
    emit updateProgressBar(100*(currentProbe+1)/total);
}

prism_progressbar::~prism_progressbar()
{
    delete ui;
}
