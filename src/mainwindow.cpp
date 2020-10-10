#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(domyostreadmill* treadmill) :
    QDialog(nullptr),
    ui(new Ui::MainWindow)
{    
    ui->setupUi(this);

    this->treadmill = treadmill;
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::update);
    timer->start(1000);

    update();
}

void MainWindow::update()
{
    ui->speed->setText(QString::number(treadmill->currentSpeed()));
    ui->inclination->setText(QString::number(treadmill->currentInclination()));
    ui->heartrate->setText(QString::number(treadmill->currentHeart()));
    if(treadmill->virtualTreadMill)
        ui->watt->setText(QString::number(treadmill->virtualTreadMill->watts(ui->weight->text().toFloat())));
    else
        ui->watt->setText("0");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addEmptyRow()
{
    int row = ui->tableWidget->rowCount();
    editing = true;
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem("00:00:00"));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem("10"));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem("0"));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(""));
    ui->tableWidget->item(row, 0)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableWidget->item(row, 1)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableWidget->item(row, 2)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableWidget->item(row, 3)->setCheckState(Qt::CheckState::Checked);
    editing = false;
}

void MainWindow::on_tableWidget_cellChanged(int row, int column)
{    
    if(editing) return;
    if(column == 0)
    {
        switch(ui->tableWidget->currentItem()->text().length())
        {
        case 4:
            ui->tableWidget->currentItem()->setText("00:0" + ui->tableWidget->currentItem()->text());
            break;
        case 5:
            ui->tableWidget->currentItem()->setText("00:" + ui->tableWidget->currentItem()->text());
            break;
        case 7:
            ui->tableWidget->currentItem()->setText("0" + ui->tableWidget->currentItem()->text());
            break;
        }
        QString fmt = "hh:mm:ss";
        QTime dt = QTime::fromString(ui->tableWidget->currentItem()->text());
        QString timeStr = dt.toString("hh:mm:ss");
        ui->tableWidget->currentItem()->setText(timeStr);
    }

    if(row + 1 == ui->tableWidget->rowCount() && ui->tableWidget->currentItem()->text().length() )
        addEmptyRow();

    QList<trainrow> rows;
    for(int i = 0; i < ui->tableWidget->rowCount(); i++)
    {
        if(!ui->tableWidget->item(i, 0)->text().contains("00:00:00"))
        {
            trainrow t;
            t.duration = QTime::fromString(ui->tableWidget->item(i, 0)->text(), "hh:mm:ss");
            t.speed = ui->tableWidget->item(i, 1)->text().toFloat();
            t.inclination = ui->tableWidget->item(i, 2)->text().toFloat();
            t.forcespeed = ui->tableWidget->item(i, 3)->checkState() == Qt::CheckState::Checked;
            rows.append(t);
        }
        else
        {
            break;
        }
        createTrainProgram(rows);
    }
}

void MainWindow::createTrainProgram(QList<trainrow> rows)
{
    if(treadmill->trainProgram) delete treadmill->trainProgram;
    treadmill->trainProgram = new trainprogram(rows);
    connect(treadmill->trainProgram, SIGNAL(start()), treadmill, SLOT(start()));
    connect(treadmill->trainProgram, SIGNAL(stop()), treadmill, SLOT(stop()));
    connect(treadmill->trainProgram, SIGNAL(changeSpeed(double)), treadmill, SLOT(changeSpeed(double)));
    connect(treadmill->trainProgram, SIGNAL(changeInclination(double)), treadmill, SLOT(changeInclination(double)));
}

void MainWindow::on_tableWidget_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void MainWindow::on_save_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                               "train.xml",
                               tr("Train Program (*.xml)"));
    if(!fileName.isEmpty() && treadmill->trainProgram)
        treadmill->trainProgram->save(fileName);
}

void MainWindow::on_load_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                               "train.xml",
                               tr("Train Program (*.xml)"));
    if(!fileName.isEmpty())
    {
        if(treadmill->trainProgram)
            delete treadmill->trainProgram;
        treadmill->trainProgram = trainprogram::load(fileName);
        int countRow = 0;
        foreach(trainrow row, treadmill->trainProgram->rows)
        {
            if(ui->tableWidget->rowCount() <= countRow)
                addEmptyRow();

            QTableWidgetItem* i;
            editing = true;
            i = ui->tableWidget->takeItem(countRow, 0);
            i->setText(row.duration.toString("hh:mm:ss"));
            ui->tableWidget->setItem(countRow, 0, i);

            i = ui->tableWidget->takeItem(countRow, 1);
            i->setText(QString::number(row.speed));
            ui->tableWidget->setItem(countRow, 1, i);

            i = ui->tableWidget->takeItem(countRow, 2);
            i->setText(QString::number(row.inclination));
            ui->tableWidget->setItem(countRow, 2, i);

            i = ui->tableWidget->takeItem(countRow, 3);
            i->setCheckState(row.forcespeed?Qt::CheckState::Checked:Qt::CheckState::Unchecked);
            ui->tableWidget->setItem(countRow, 3, i);

            editing = false;

            countRow++;
        }
    }
}

void MainWindow::on_reset_clicked()
{
    if(treadmill->currentSpeed() > 0) return;

    int countRow = 0;
    foreach(trainrow row, treadmill->trainProgram->rows)
    {
        QTableWidgetItem* i;
        editing = true;
        i = ui->tableWidget->takeItem(countRow, 0);
        i->setText("00:00:00");
        ui->tableWidget->setItem(countRow, 0, i);

        i = ui->tableWidget->takeItem(countRow, 1);
        i->setText("0");
        ui->tableWidget->setItem(countRow, 1, i);

        i = ui->tableWidget->takeItem(countRow, 2);
        i->setText("0");
        ui->tableWidget->setItem(countRow, 2, i);

        i = ui->tableWidget->takeItem(countRow, 3);
        i->setCheckState(row.forcespeed?Qt::CheckState::Checked:Qt::CheckState::Unchecked);
        ui->tableWidget->setItem(countRow, 3, i);

        editing = false;

        countRow++;
    }

    createTrainProgram(QList<trainrow>());
}

void MainWindow::on_stop_clicked()
{
    treadmill->stop();
}

void MainWindow::on_start_clicked()
{
    treadmill->trainProgram->restart();
    treadmill->start();
}