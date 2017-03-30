#include <stan/services/sample/hmc_nuts_dense_e_adapt.hpp>
#include <stan/services/sample/hmc_nuts_dense_e.hpp>
#include <gtest/gtest.h>
#include <stan/io/empty_var_context.hpp>
#include <test/test-models/good/mcmc/hmc/common/gauss3D.hpp>
#include <test/unit/services/instrumented_callbacks.hpp>
#include <iostream>
#include <boost/algorithm/string.hpp>

/** 
 * Use model with 3 params, fix seed, set mass matrix
 */

class ServicesSampleHmcNutsDenseEMassMatrix : public testing::Test {
public:
  ServicesSampleHmcNutsDenseEMassMatrix()
    : model(context, &model_log) {}

  std::stringstream model_log;
  stan::test::unit::instrumented_writer message, init, error;
  stan::test::unit::instrumented_writer parameter, diagnostic;
  stan::io::empty_var_context context;
  stan_model model;
};

void check_adaptation(const size_t& num_params,
                      const std::vector<double>& dense_vals,
                      stan::test::unit::instrumented_writer& report) {

  std::vector<std::string> param_strings = report.string_values();
  size_t offset = 0;
  for (size_t i = 0; i < param_strings.size(); i++) {
    offset++;
    if (param_strings[i].find("Elements of inverse mass matrix:")
        != std::string::npos) {
      break;
    }
  }
  for (size_t i = offset, ij=0 ; i < offset + num_params; i++) {
      std::vector<std::string> strs;
      boost::split(strs, param_strings[i], boost::is_any_of(", "), boost::token_compress_on);
      EXPECT_EQ(num_params, strs.size());
      for (size_t j = 0; j < num_params; ij++, j++) {
        //        std::cout << " ij " << ij << " " << dense_vals[ij] << " " << strs[j] << std::endl;
        ASSERT_NEAR(dense_vals[ij], std::stod(strs[j]), 0.05);
      }
  }
}

TEST_F(ServicesSampleHmcNutsDenseEMassMatrix, no_adapt) {
  unsigned int random_seed = 12345;
  unsigned int chain = 1;
  double init_radius = 2;
  int num_warmup = 0;
  int num_samples = 2;
  int num_thin = 1;
  bool save_warmup = false;
  int refresh = 0;
  double stepsize = 1;
  double stepsize_jitter = 0;
  int max_depth = 10;
  stan::test::unit::instrumented_interrupt interrupt;
  EXPECT_EQ(interrupt.call_count(), 0);

  // mass matrix from 250 warmups, seed = 12345
  // resulting step size: 0.60
  std::string txt =
    "mass_matrix <- structure(c("
    "0.640211, 0.156096, -0.374048, "
    "0.156096, 1.41239, -0.0412753, "
    "-0.374048, -0.0412753, 1.29567 "
    "), .Dim  = c(3,3))";

  std::stringstream in(txt);
  stan::io::dump dump(in);
  stan::io::var_context& inv_mass_matrix = dump;
  size_t num_elements = 9;
  std::vector<double> dense_vals(num_elements);
  dense_vals = inv_mass_matrix.vals_r("mass_matrix");

  int return_code =
    stan::services::sample::hmc_nuts_dense_e(
    model, context, inv_mass_matrix, random_seed, chain, init_radius,
    num_warmup, num_samples, num_thin, save_warmup, refresh,
    stepsize, stepsize_jitter, max_depth,
    interrupt, message, error, init,
    parameter, diagnostic);
  EXPECT_EQ(0, return_code);

  // check returned mass matrix
  check_adaptation(3, dense_vals, parameter);
}


TEST_F(ServicesSampleHmcNutsDenseEMassMatrix, skip_adapt) {
  unsigned int random_seed = 12345;
  unsigned int chain = 1;
  double init_radius = 2;
  int num_warmup = 0;
  int num_samples = 2;
  int num_thin = 1;
  bool save_warmup = false;
  int refresh = 0;
  double stepsize = 1;
  double stepsize_jitter = 0;
  int max_depth = 10;
  double delta = .8;
  double gamma = .05;
  double kappa = .75;
  double t0 = 10;
  unsigned int init_buffer = 75;
  unsigned int term_buffer = 50;
  unsigned int window = 25;
  stan::test::unit::instrumented_interrupt interrupt;
  EXPECT_EQ(interrupt.call_count(), 0);

  // mass matrix from 250 warmups, seed = 12345
  // resulting step size: 0.60
  std::string txt =
    "mass_matrix <- structure(c("
    "0.640211, 0.156096, -0.374048, "
    "0.156096, 1.41239, -0.0412753, "
    "-0.374048, -0.0412753, 1.29567 "
    "), .Dim  = c(3,3))";

  std::stringstream in(txt);
  stan::io::dump dump(in);
  stan::io::var_context& inv_mass_matrix = dump;
  size_t num_elements = 9;
  std::vector<double> dense_vals(num_elements);
  dense_vals = inv_mass_matrix.vals_r("mass_matrix");

  int return_code =
    stan::services::sample::hmc_nuts_dense_e_adapt(
    model, context, inv_mass_matrix, random_seed, chain, init_radius,
    num_warmup, num_samples, num_thin, save_warmup, refresh,
    stepsize, stepsize_jitter, max_depth, delta, gamma, kappa, t0,
    init_buffer, term_buffer, window,
    interrupt, message, error, init,
    parameter, diagnostic);

  EXPECT_EQ(0, return_code);
  check_adaptation(3, dense_vals, parameter);
}

// run model for 2000 iterations, starting w/ dense matrix from running 250
// at this point, all 3 params should be very close to 1
TEST_F(ServicesSampleHmcNutsDenseEMassMatrix, continue_adapt) {
  unsigned int random_seed = 12345;
  unsigned int chain = 1;
  double init_radius = 2;
  int num_warmup = 2000;
  int num_samples = 0;
  int num_thin = 1;
  bool save_warmup = false;
  int refresh = 0;
  double stepsize = 1;
  double stepsize_jitter = 0;
  int max_depth = 10;
  double delta = .8;
  double gamma = .05;
  double kappa = .75;
  double t0 = 10;
  unsigned int init_buffer = 75;
  unsigned int term_buffer = 50;
  unsigned int window = 25;
  stan::test::unit::instrumented_interrupt interrupt;
  EXPECT_EQ(interrupt.call_count(), 0);

  // mass matrix from 250 warmups, seed = 12345
  // resulting step size: 0.60
  std::string txt =
    "mass_matrix <- structure(c("
    "0.640211, 0.156096, -0.374048, "
    "0.156096, 1.41239, -0.0412753, "
    "-0.374048, -0.0412753, 1.29567 "
    "), .Dim  = c(3,3))";

  std::stringstream in(txt);
  stan::io::dump dump(in);
  stan::io::var_context& inv_mass_matrix = dump;

  int return_code =
    stan::services::sample::hmc_nuts_dense_e_adapt(
    model, context, inv_mass_matrix, random_seed, chain, init_radius,
    num_warmup, num_samples, num_thin, save_warmup, refresh,
    stepsize, stepsize_jitter, max_depth, delta, gamma, kappa, t0,
    init_buffer, term_buffer, window,
    interrupt, message, error, init,
    parameter, diagnostic);

  EXPECT_EQ(0, return_code);

  // 2000 warmup steps should push matrix to ident
  std::vector<double> dense_vals(9);
  for (size_t i=0; i<9; i++) {
    dense_vals[i] = 0.00;
  }
  // diagonals are 1
  dense_vals[0] = 1.00;
  dense_vals[4] = 1.00;
  dense_vals[8] = 1.00;
  check_adaptation(3, dense_vals, parameter);
}
