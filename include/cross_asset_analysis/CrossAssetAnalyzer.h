#ifndef CROSS_ASSET_ANALYZER_H
#define CROSS_ASSET_ANALYZER_H

#include <vector>
#include <string>
#include <dlib/matrix.h>

namespace cross_asset_analysis {

using matrix_type = dlib::matrix<double>;

class CrossAssetAnalyzer {
public:
    CrossAssetAnalyzer(const std::map<std::string, std::vector<double>>& asset_returns);
    void calculate_correlation_matrix();
    void perform_regime_analysis();

    const matrix_type& get_correlation_matrix() const;

private:
    std::map<std::string, std::vector<double>> asset_returns_;
    matrix_type correlation_matrix_;
};

}

#endif // CROSS_ASSET_ANALYZER_H
