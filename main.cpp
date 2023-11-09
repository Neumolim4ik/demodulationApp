#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QDataStream>
#include <complex>
#include <QStatusBar>
#include <QtCore>
#include <QtWidgets>
#include <QtMultimedia>
#include <QAudioOutput>
#include <QAudioFormat>
#include <iostream>
#include <QtGui>
#include <cmath>
#include <fstream>
#include <vector>


template<typename T>
std::vector<T> readBinaryFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(nullptr, "Error", "Failed to open file: " + file.errorString());
        return {};
    }

    qint64 fileSize = file.size();

    qint64 numElements = fileSize / sizeof(T);

    QByteArray buffer = file.readAll();
    if (buffer.size() != fileSize)
    {
        QMessageBox::critical(nullptr, "Error", "Failed to read file: " + file.errorString());
        return {};
    }

    std::vector<T> result(numElements);
    std::memcpy(result.data(), buffer.data(), fileSize);

    return result;
}


void AMdemodulateData(QVector<double>* data, QVector<double>* demodulatedData) {
    QVector<double> ReS, ImS, A;
    QVector<std::complex<double>> S;

    for (int i = 0; i < data->size() / 2; i++) {
        ReS.push_back(data->at(2 * i));
        ImS.push_back(data->at(2 * i + 1));
    }

    for (int i = 0; i < ReS.size(); i++) {
        S.append(std::complex<double>(ReS[i], ImS[i]));
        A.append(std::abs(S[i]));
    }
    ReS.clear();
    ImS.clear();
    double M = std::accumulate(A.begin(), A.end(), 0.0) / A.size();

    demodulatedData->resize(A.size());

    for (int i = 0; i < A.size(); ++i) {
        (*demodulatedData)[i] = (A[i] - M);
    }


}

void FMdemodulateData(QVector<double>* data, QVector<double>* demodulatedData, int sampleRate) {
    QVector<double> ReS, ImS, psi, y_fm ;
    // Заполнение векторов ReS и ImS соответственно
    for (int i = 0; i < data->size() / 2; i++) {
        ReS.push_back(data->at(2 * i));
        ImS.push_back(data->at(2 * i + 1));
    }

    // Вычисление фазы комплексного сигнала
    psi.resize(ImS.size());
    for (int i = 0; i < ImS.size(); i++) {
        psi[i] = std::arg(std::complex<double>(ReS[i], ImS[i]));
    }

    // Вычисление частоты комплексного сигнала
    y_fm.resize(psi.size() - 1);
    for (int i = 0; i < psi.size() - 1; i++) {
        y_fm[i] = (psi[i + 1] - psi[i]) * sampleRate;
    }

    // Фильтр скользящего среднего
    int windowSize = 32;
    QVector<double> b(windowSize, 1.0 / windowSize);
    QVector<double> a(1, 1.0);
    QVector<double> z_fm(y_fm.size());

    for (int i = 0; i < y_fm.size(); i++) {
        z_fm[i] = 0.0;
        for (int j = 0; j < windowSize; j++) {
            if (i - j >= 0) {
                z_fm[i] += b[j] * y_fm[i - j];
            }
        }
    }

    // Передискретизация
    QVector<double> downsampled_z_fm;
    int decimationFactor = 16;
    for (int i = 0; i < z_fm.size(); i += decimationFactor) {
        downsampled_z_fm.push_back(z_fm[i]);
    }

    // Запись результата в demodulatedData
    demodulatedData->clear();
    for (int i = 0; i < downsampled_z_fm.size(); i++) {
        demodulatedData->push_back(downsampled_z_fm[i]);
    }
}


void saveData(const QString &filename, QVector<double>* demodulatedData) {

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file" << filename << "for writing:" << file.errorString();
        return;
    }

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    // ChunkID и Format
    out.writeRawData("RIFF", 4);
    quint32 fileSize = 0;
    out << fileSize;
    out.writeRawData("WAVE", 4);

    // Subchunk1ID и Subchunk1Size
    out.writeRawData("fmt ", 4);
    quint32 subchunk1Size = 16;
    out << subchunk1Size;
    quint16 audioFormat = 1;
    out << audioFormat;
    quint16 numChannels = 1;
    out << numChannels;
    quint32 sampleRate = 16000;
    out << sampleRate;
    quint32 byteRate = sampleRate * numChannels * 2;
    out << byteRate;
    quint16 blockAlign = numChannels * 2;
    out << blockAlign;
    quint16 bitsPerSample = 16;
    out << bitsPerSample;


    out.writeRawData("data", 4);
    quint32 dataSize = demodulatedData->size() * 2;
    out << dataSize;

    // Записываем данные
    for (int i = 0; i < demodulatedData->size(); i++) {
        qint16 sample = static_cast<qint16>(demodulatedData->at(i));
        out << sample;
    }

    // Обновляем размер файла
    fileSize = file.size() - 8;
    file.seek(4);
    out << fileSize;

    file.close();


}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QVector<double>* data = new QVector<double>;
    QVector<double>* demodulatedData = new QVector<double>;
    QWidget *mainWindow = new QWidget();
    mainWindow->setWindowTitle("Radio Signal Demodulator");

    QGridLayout *layout = new QGridLayout();

    QPushButton *loadButton1 = new QPushButton("Load File int32_t");
    QPushButton *loadButton2 = new QPushButton("Load File float");
    QPushButton *AMdemodButton = new QPushButton("Demodulate AM");
    QPushButton *FMdemodButton = new QPushButton("Demodulate FM");
    QPushButton *saveButton = new QPushButton("Save File");

    layout->addWidget(loadButton1, 0, 0);
    layout->addWidget(loadButton2, 1, 0);
    layout->addWidget(AMdemodButton, 0, 1);
    layout->addWidget(FMdemodButton, 1, 1);
    layout->addWidget(saveButton, 0, 2);

    mainWindow->setLayout(layout);
    mainWindow->show();

    QObject::connect(loadButton1, &QPushButton::clicked, [=]() {
        QString filename = QFileDialog::getOpenFileName(mainWindow, "Load File", "", "Binary Files (*.dat)");
        if (!filename.isEmpty()) {

            auto dataInt = readBinaryFile<int32_t>(filename);

            for (auto value : dataInt) {
                double doubleValue = static_cast<double>(value); // преобразование int32_t в float
                data->append(doubleValue);
            }
        }
    });

    QObject::connect(loadButton2, &QPushButton::clicked, [=]() {
        QString filename = QFileDialog::getOpenFileName(mainWindow, "Load File", "", "Binary Files (*.bin)");
        if (!filename.isEmpty()) {

            auto dataInt = readBinaryFile<float>(filename);

            for (auto value : dataInt) {
                double doubleValue = static_cast<double>(value); // преобразование int32_t в float
                data->append(doubleValue);
            }

        }
    });

    QObject::connect(AMdemodButton, &QPushButton::clicked, [=]() {


        AMdemodulateData(data,demodulatedData);
        delete data;

    });
    QObject::connect(FMdemodButton, &QPushButton::clicked, [=]() {


        FMdemodulateData(data,demodulatedData, 16000);
        delete data;

    });

    QObject::connect(saveButton, &QPushButton::clicked, [=]() {
        // Get the data to save
        QString filename = QFileDialog::getSaveFileName(mainWindow, "Save File", "", "Wave Files (*.wav");
        if (!filename.isEmpty()) {

            // Save the data to the selected file
            saveData(filename, demodulatedData);
            delete demodulatedData;
        }
    });


    return app.exec();
}
