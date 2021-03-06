/**
 * @file place_util.h
 * @brief Utility structures representing various states of the
 *        placement. Also contains declarations of related routines.
 */

#pragma once
#include "vpr_types.h"

/**
 * @brief Data structure that stores different cost values in the placer.
 *
 * Although we do cost calculations with float values, we use doubles
 * for the accumulated costs to avoid round-off, particularly on large
 * designs where the magnitude of a single move's delta cost is small
 * compared to the overall cost.
 *
 * To balance the trade-off between timing and wirelength (bb) cost, the
 * change in costs produced by block swaps are divided by the final cost
 * values of the previous iteration. However, the divisions are expensive,
 * so we store their multiplicative inverses when they are updated in
 * the outer loop routines to speed up the normalization process.
 *
 *   @param cost The weighted average of the wiring cost and the timing cost.
 *   @param bb_cost The bounding box cost, aka the wiring cost.
 *   @param timing_cost The timing cost, which is connection delay * criticality.
 *
 *   @param bb_cost_norm The normalization factor for the wiring cost.
 *   @param timing_cost_norm The normalization factor for the timing cost, which
 *              is upper-bounded by the value of MAX_INV_TIMING_COST.
 *
 *   @param MAX_INV_TIMING_COST Stops inverse timing cost from going to infinity
 *              with very lax timing constraints, which avoids multiplying by a
 *              gigantic timing_cost_norm when auto-normalizing. The exact value
 *              of this cost has relatively little impact, but should not be large
 *              enough to be on the order of timing costs for normal constraints.
 *
 *   @param place_algorithm Determines how the member values are updated upon
 *              each temperature change during the placer annealing process.
 */
class t_placer_costs {
  public: //members
    double cost;
    double bb_cost;
    double timing_cost;
    double bb_cost_norm;
    double timing_cost_norm;

  public: //Constructor
    t_placer_costs(t_place_algorithm algo)
        : place_algorithm(algo) {}

  public: //Mutator
    void update_norm_factors();

  private:
    double MAX_INV_TIMING_COST = 1.e9;
    t_place_algorithm place_algorithm;
};

/**
 * @brief Stores variables that are used by the annealing process.
 *
 * This structure is updated by update_annealing_state() on each outer
 * loop iteration. It stores various important variables that need to
 * be accessed during the placement inner loop.
 *
 * Public members:
 *   @param t
 *              Temperature for simulated annealing.
 *   @param rlim
 *              Range limit for block swaps.
 *   @param alpha
 *              Temperature decays factor (multiplied each outer loop iteration).
 *   @param restart_t
 *              Temperature used after restart due to minimum success ratio.
 *   @param crit_exponent
 *              Used by timing-driven placement to "sharpen" the timing criticality.
 *   @param move_lim_max
 *              Maximum block move limit.
 *   @param move_lim
 *              Current block move limit.
 *
 * Private members:
 *   @param UPPER_RLIM
 *              The upper limit for the range limiter value.
 *   @param FINAL_RLIM
 *              The final rlim (range limit) is 1, which is the smallest value that
 *              can still make progress, since an rlim of 0 wouldn't allow any swaps.
 *   @param INVERSE_DELTA_RLIM
 *              Used to update crit_exponent. See update_rlim() for more.
 *
 * Mutators:
 *   @param outer_loop_update()
 *              Update the annealing state variables in the placement outer loop.
 *   @param update_rlim(), update_crit_exponent(), update_move_lim()
 *              Inline subroutines used by the main routine outer_loop_update().
 */
class t_annealing_state {
  public:
    float t;
    float rlim;
    float alpha;
    float restart_t;
    float crit_exponent;
    int move_lim_max;
    int move_lim;
    float success_rate;
    int num_temps = 0;

  private:
    float UPPER_RLIM;
    float FINAL_RLIM = 1.;
    float INVERSE_DELTA_RLIM;

  public: //Constructor
    t_annealing_state(const t_annealing_sched& annealing_sched,
                      float first_t,
                      float first_rlim,
                      int first_move_lim,
                      float first_crit_exponent);

  public: //Mutator
    bool outer_loop_update(const t_placer_costs& costs,
                           const t_placer_opts& placer_opts,
                           const t_annealing_sched& annealing_sched);

  private: //Mutator
    inline void update_rlim();
    inline void update_crit_exponent(const t_placer_opts& placer_opts);
    inline void update_move_lim(float success_target);
};

///@brief Initialize the placement context.
void init_placement_context();

///@brief Get the initial limit for inner loop block move attempt limit.
int get_initial_move_lim(const t_placer_opts& placer_opts, const t_annealing_sched& annealing_sched);

///@brief Returns the standard deviation of data set x.
double get_std_dev(int n, double sum_x_squared, double av_x);