#pragma once

#include <random> // normal_distribution
#include <iostream>
#include <fstream>
#include "matrix.cpp"
#include "evaluate.cpp"


using namespace std;


/*** Requisites ***/
vector<double> softmax(const vector<double>& v) {
    // Subtract maximum to avoid overflow
    double max = *max_element(v.begin(), v.end());

    vector<double> expd = v; // copy, does not mutate v
    double sum = 0;
    for (double& x : expd) {
        x = exp(x - max);
        sum += x;
    }

    vector<double> result(v.size(), 0.);
    for (int i = 0; i < v.size(); ++i)
        result[i] = expd[i] / sum;

    return result;
}
Matrix softmax(const Matrix& m) {
    Matrix result = m;
    for (auto& row : result)
        row = softmax(row);
    return result;
}

double cross_entropy(const Matrix& Y, const Matrix& Y_prob) {
    // Cost function
    Matrix result = hadamard(Y, log(Y_prob));
    return 1/double(n_rows(Y)) * sum(result);
}

Matrix random_init(int rows, int cols, double mean=0., double std=1.) {
    default_random_engine generator;
    normal_distribution<double> distribution(mean, std);

    Matrix result = blank_matrix(rows, cols, 0.);
    for (auto& row : result)
        for (double& elem : row)
            elem = distribution(generator);

    return result;
}




class NeuralNetwork {

    int n_classes;
    int data_dim;
    Matrix W;
    Matrix b;

    Matrix Y_prob;
    Matrix delta;
    Matrix grad_W;
    Matrix grad_b;

    Matirx XW;
    Matrix XWb;
    Matrix Y_prob;

    Matrix delta;

    Matrix trX;
    Matrix trXdelta;
    Matrix grad_W;

    Matrix delta_sums;

    Matrix chunk_X(chunk_size, 784);


public:

    NeuralNetwork(int n_classes=10, int data_dim=784)
            : n_classes(n_classes), data_dim(data_dim) {
        b = blank_matrix(1, n_classes, 0.);
        W = random_init(data_dim, n_classes);
        // TODO? more layers
    }

    NeuralNetwork(string path) {
        this->load(path);
    }

    void grad(Matrix X, Matrix Y, Matrix& grad_W, Matrix& grad_b) {
        dot(X, W, XW); // XW = X * W
        add_to_each(XW, b, XWb); // XWb = X * W + b
        softmax(XWb, Y_prob); // softmaxed = softmax(X * W + b)
        sub(Y_prob, Y, delta); // delta = Y_prob - Y

        double contribution = 1/double(X.n_rows);

        transpose(X, trX); // trX = transpose(X)
        dot(trX, delta, trXdelta); // trXdelta = transpose(X) * delta
        scalar_mult(contribution, trXdelta, grad_W); // grad_W = 1/n * transpose(X) * delta


        col_wise_sums(delta, delta_sums);
        scalar_mult(contribution, delta_sums); // grad_b = 1/n * col_wise_sums(delta)

        // TODO maybe
//        return cross_entropy(Y, Y_prob); // cost
    }

    void train(Matrix X, Matrix Y,
               vector<double>& cost_history, vector<double>& accuracy_history,
               int n_epochs=10, int batch_size=100, double lr=0.1,
               bool verbose=true) {
        double cost, acc;
        auto Y_labels = from_one_hot_matrix(Y); // {5, 2, 9, ... }

//        Matrix grad_W = blank_matrix(data_dim, n_classes, 0.);
//        Matrix grad_b = blank_matrix(1, n_classes, 0.);

        for (int epoch = 1; epoch <= n_epochs; ++epoch) {
            if (verbose)
                cout << string_format("Epoch %d / %d", epoch, n_epochs) << endl;


            for (int i = 0; i < n_rows(X); i+=batch_size) {
                // TODO: make random batch generator

                cost = grad(chunk(X, i, i+batch_size), chunk(Y, i, i+batch_size),
                            grad_W,  grad_b);

                // TODO regularization
                W = W - lr * grad_W;
                b = b - lr * grad_b;

                cost_history.push_back(cost);
            }
            // TODO: add early stopping

            // Compute accuracy after each epoch
            acc = accuracy(predict(X), Y_labels);
            accuracy_history.push_back(acc);
        }
    }

    vector<int> predict(const Matrix& X) {
        Matrix Y_prob = softmax(X * W + b);
        return argmax_matrix(Y_prob);
    }

    int predict_one(const vector<double>& pixels, double& confidence) {
        // TODO: refactor to use .predict()
        Matrix X = {pixels};
        vector<double> y_prob = softmax(X * W + b)[0]; // just one image, take the first row

        int digit_predicted = argmax(y_prob);
        confidence = y_prob[digit_predicted];
        return digit_predicted;
    }


    /*** Serialization ***/
    void save(string path) {
        ofstream file(path);
        if (!file.is_open())
            throw runtime_error(string_format("Could not open for model save: " + path));

        file << *this;

        file.close();
    }

    void load(string path) {
        ifstream file(path);
        if (!file.is_open())
            throw runtime_error(string_format("Could not open for model load: " + path));

        file >> *this;

        file.close();
    }

    friend ostream& operator<<(ostream& os, const NeuralNetwork& net);
    friend istream& operator>>(istream& is, NeuralNetwork& net);
};


ostream& operator<<(ostream& os, const NeuralNetwork& net) {
    os << net.n_classes << ' ' << net.data_dim << endl;
    os << net.b << net.W;  // place W last so it can be read until end of file
    return os;
}

istream& operator>>(istream& is, NeuralNetwork& net) {
    is >> net.n_classes >> net.data_dim;

    // net.b is a 1xn matrix, if we read a matrix normally, we read until end of file,
    // but a vector is read until end of file
    vector<double> b_vect;
    do {
        is >> b_vect;
    } while (b_vect.size() == 0);  // to account for cursor left at end of line
    net.b = {b_vect};

    is >> net.W;
    return is;
}