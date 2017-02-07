#include <iostream>
#include "reading.cpp"
#include "nn.cpp"
#include <iomanip>

using namespace std;


void train_and_save() {
    // TODO: names as additional parameters
    Matrix images, labels;
    read_data("data/train.csv", images, labels);

    NeuralNetwork net;
    vector<double> costs, accs;
    net.train(images, labels, costs, accs, 35, 100, .1);

    net.save("models/trained.nn");
    ofstream f("histories/training.txt");
    f << costs << endl << accs;
    f.close();
}

int predict(string pixels_string) {
    // TODO: names as additional parameters
    vector<double> pixels = pixels_from_string(pixels_string);
    NeuralNetwork net("models/trained_a.nn");

    double confidence;
    int digit_predicted = net.predict_one(pixels, confidence);
    cout << digit_predicted << endl << confidence << endl;

    return digit_predicted;
}

int main(int argc, char* argv[]) {
    Matrix images, labels;
    read_data("/Users/Stefan/Projects/mnist-in-mpi/data/train.csv", images, labels, true);
    NeuralNetwork net;
    vector<double> costs, accs;
    cout << setprecision(3);
    net.train(images, labels, costs, accs, 35, 400, .1);
    return 0;

    if (argc < 2) {
        cout << "usage: mnist_in_mpi.out (train|predict)" << endl;
        return 1;
    }

    if (string(argv[1]) == "train") {
        train_and_save();
        return 0;
    }

    if (string(argv[1]) == "predict") {
        if (argc < 3) {
            cout << "usage mnist_in_mpi.out predict <pixels>" << endl;
            return 1;
        }
        predict(argv[2]);
        return 0;
    }
}
