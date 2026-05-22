#include<iostream>
#include<vector>
#include<string>
#include <fstream>
#include <sstream>

using namespace std;

class stockRecord{
    public:
    string date;
    double closing_price;

    stockRecord(string d, double price):date(d), closing_price(price){}

    string getDate() const {
        return date;
    }
    double getClosePrice() const {
        return closing_price;
    }

};

class stockModel{
    public:
    vector<stockRecord> historical_data;
    stockModel(vector<stockRecord>& data){
        historical_data = data;
    }
    //////
    virtual ~stockModel() = default;
    virtual void fit() = 0;
    virtual double predict(int futureStep) const = 0;
    //////


};

class linearRegression : public stockModel{
    public:
    double m = 0.0;
    double c = 0.0;
    
    linearRegression(vector<stockRecord>& data):stockModel(data){}
    void fit() override{
        int N = historical_data.size();
        if(N==0){
            return;
        }
        double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumXsq = 0.0;
        for(int i=0; i<N; i++){
            double x = i;
            double y = historical_data[i].getClosePrice();
            sumX += x;
            sumY += y;
            sumXY += (x*y);
            sumXsq += (x*x);

        }

        double Denominator = (N*sumXsq) - (sumX * sumX);
        if(Denominator!=0){
            m = ((N*sumXY)-(sumX*sumY))/Denominator;
            c = (sumY - (m*sumX))/N;
        }

    }

    double predict(int futurestep) const override{
        return (m*futurestep) + c;
    }
};

class StockParser {
public:
    static vector<stockRecord> parseCSV(const string& filename) {
        vector<stockRecord> records;
        ifstream file(filename);
        
        if (!file.is_open()) {
            cerr << "Error: Could not open the file " << filename << endl;
            return records;
        }

        string line;
        if (!getline(file, line)) {
            return records;
        }

        while (getline(file, line)) {
            if (line.empty()) continue; 

            stringstream rowStream(line);
            string cell;
            
            string date = "";
            double closePrice = 0.0;
            int currentColumn = 0;

            while (getline(rowStream, cell, ',')) {
                if (currentColumn == 0) {
                    date = cell;
                } 
                else if (currentColumn == 1) {
                    try {
                        closePrice = stod(cell); // Converts string to double
                    } catch (const exception& e) {
                        //cerr << "Warning: Corrupt price data skipped on date: " << date << endl;
                        continue; 
                    }
                }
                currentColumn++;
            }
            records.push_back(stockRecord(date, closePrice));
        }

        file.close();
        return records;
    }
};

class StockVisualizer {
public:
    // Takes our data vector, our analytical model, and how many days into the future we want to forecast
    static void plotData(const vector<stockRecord>& historicalData, const stockModel* model, int forecastDays) {
        // 1. Export both historical and forecast datasets into a single text file
        ofstream dataFile("plot_temp.dat");
        if (!dataFile.is_open()) {
            cerr << "Error: Could not generate temporary plot file." << endl;
            return;
        }

        int N = historicalData.size();

        for (int i = 0; i < N; ++i) {
            dataFile << i << " " << historicalData[i].getClosePrice() << endl;
        }
        dataFile.close();

        ofstream predFile("pred_temp.dat");
        if (!predFile.is_open()) {
            cerr << "Error: Could not generate temporary prediction file." << endl;
            return;
        }

        for (int i = N - 1; i < N + forecastDays; ++i) {
            double predictedValue = model->predict(i);
            predFile << i << " " << predictedValue << endl;
        }
        predFile.close();

        // 2. Open a pipe command to send styling instructions to Gnuplot
        // For Windows environments: use "_popen" instead of "popen"
        FILE* gnuplotPipe = popen("gnuplot -persist", "w");
        
        if (gnuplotPipe == nullptr) {
            cerr << "Error: Gnuplot not found! Please verify it is installed and added to your system PATH environment variables." << endl;
            return;
        }

        // 3. Stream formatting instructions through our active pipeline
        fprintf(gnuplotPipe, "set title 'Price Predictor Project'\n");
        fprintf(gnuplotPipe, "set xlabel 'Trading Day Tracking Index'\n");
        fprintf(gnuplotPipe, "set ylabel 'Price per Share ($)'\n");
        fprintf(gnuplotPipe, "set grid\n");

        // Draw both files: line style 1 (blue) for history, line style 2 (red dashed) for prediction
        fprintf(gnuplotPipe, "plot 'plot_temp.dat' with lines title 'Historical Market Closing Price' lw 2 lc rgb 'blue', \\\n");
        fprintf(gnuplotPipe, "     'pred_temp.dat' with lines title 'Linear Regression Forecast Line' lw 2 lc rgb 'red' dt 2\n");

        // 4. Terminate pipeline stream smoothly
        pclose(gnuplotPipe);
    }
};

int main(){
    string filePath = "AAPL_historical_data.csv"; 
    vector<stockRecord> realStockData = StockParser::parseCSV(filePath);

    if (realStockData.empty()) {
        cerr << "Execution aborted: No input records available." << endl;
        return 1;
    }

    // Phase 2: Core Algorithm Configuration
    stockModel* analyticsEngine = new linearRegression(realStockData);
    analyticsEngine->fit();

    // Phase 3: Visual Rendering & Prediction Tracking
    int forecastingWindow = 30; // Attempt to project the trend line out 30 days ahead
    
    cout << "Launching Gnuplot visual environment rendering..." << endl;
    StockVisualizer::plotData(realStockData, analyticsEngine, forecastingWindow);

    // Clean up dynamic allocations
    delete analyticsEngine;
    return 0;
}