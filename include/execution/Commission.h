#ifndef COMMISSION_H
#define COMMISSION_H

#include <stdexcept>

/**
 * @class Commission
 * @brief Abstract base class for commission models.
 *
 * Provides an interface for calculating the cost of a transaction.
 */
class Commission {
public:
    virtual ~Commission() = default;

    /**
     * @brief Calculates the commission for a given transaction.
     * @param quantity The number of units transacted.
     * @param price The price per unit.
     * @return The total commission fee for the transaction.
     */
    virtual double calculate(double quantity, double price) const = 0;
};

/**
 * @class FixedCommission
 * @brief A commission model that calculates a fixed fee per trade.
 *
 * This is a simple model where every trade incurs the same commission fee,
 * regardless of size or value.
 */
class FixedCommission : public Commission {
public:
    /**
     * @brief Construct a new Fixed Commission object
     * @param fee The fixed fee per trade. Defaults to 0.0.
     */
    explicit FixedCommission(double fee = 0.0) : fee_per_trade_(fee) {}

    /**
     * @brief Calculates the commission for a transaction.
     * @param quantity The number of units (ignored in this model).
     * @param price The price per unit (ignored in this model).
     * @return The fixed commission fee.
     */
    double calculate(double quantity, double price) const override {
        // Unused parameters
        (void)quantity;
        (void)price;
        return fee_per_trade_;
    }

private:
    double fee_per_trade_;
};

#endif // COMMISSION_H