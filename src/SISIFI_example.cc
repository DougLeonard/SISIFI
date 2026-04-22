// SISIFI_example.cc by D.S. Leonard April 2026, MIT
// No AI code in this.
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class IVehicleBuilder;
class CarBuilder;
class BikeBuilder;
// Fluent pointer return types:
using IVehicleBuilderPtr = std::shared_ptr<IVehicleBuilder>;
using CarBuilderPtr = std::shared_ptr<CarBuilder>;
using BikeBuilderPtr = std::shared_ptr<BikeBuilder>;

//A SISIFI inheritance tag:
#define SISIFI_TAG std::integral_constant<int,0> = {}


class IVehicleBuilder {
public:
    virtual ~IVehicleBuilder() = default;

    // linter complains about default argument in virtual functions, but we know what we're doing.
    virtual IVehicleBuilderPtr SetDisplacement(int cc, SISIFI_TAG) = 0;
    virtual IVehicleBuilderPtr SetNumWheels(int numwheels, SISIFI_TAG) = 0;
    virtual IVehicleBuilderPtr Build(SISIFI_TAG) = 0;
};

template<typename Concrete>
class IVehicleBuilderImpl : public IVehicleBuilder, public std::enable_shared_from_this<Concrete> {
public:
    //Subscribe to the inheriticane, delegate to the business, return the interface type:
    IVehicleBuilderPtr SetDisplacement(int cc, SISIFI_TAG) final {
        static_cast<Concrete *>(this)->SetDisplacement(cc);
        return this->shared_from_this();
    }

    // final prevents accidental inheritance and helps devirtualization:
    IVehicleBuilderPtr SetNumWheels(int num_wheels, SISIFI_TAG) final {
        static_cast<Concrete *>(this)->SetNumWheels(num_wheels);
        return this->shared_from_this();
    }

    IVehicleBuilderPtr Build(SISIFI_TAG) final {
        static_cast<Concrete *>(this)->Build();
        return this->shared_from_this();
    }
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
template<typename Concrete>
class VehicleBuilder : public IVehicleBuilderImpl<Concrete> {
    using ConcretePtr = std::shared_ptr<Concrete>;
    int displacement_cc_ = 100;

protected:
    int num_wheels = 0;

    virtual std::string GetBody() = 0;

public:
    //******************* HERE IS THE MAGIC.   ********************************************
    // We opt out of the inheritance by dropping the SISIFI_TAG and can have a different return type!
    ConcretePtr SetDisplacement(int cc) {
        displacement_cc_ = cc;
        return this->shared_from_this();
    }

    // A GOTCHA is you should re-mark it as virtual here if you need polymorphism to the concrete.
    // Without virtual, this is now just a normal method, like above.
    virtual ConcretePtr SetNumWheels(int num_wheels) = 0;

    ConcretePtr Build() {
        std::cout << ">>> This is a vehicle with "
                << displacement_cc_ << " cc displacement, "
                << num_wheels << " wheels,  and a "
                << this->GetBody() << " \n";
        if (num_wheels == 0) {
            std::cout << "You should add some wheels." << std::endl;
        };
        return this->shared_from_this();
    }
};
#pragma GCC diagnostic pop

class CarBuilder : public VehicleBuilder<CarBuilder> {
    int num_doors_ = 4;

public:
    CarBuilderPtr SetNumDoors(int doors) {
        num_doors_ = doors;
        return this->shared_from_this();
    }

    CarBuilderPtr SetNumWheels(int num_wheels) final {
        if (num_wheels != 4) { std::cout << "Cars have 4 wheels, silly!" << std::endl; }
        this->num_wheels = 4;
        return this->shared_from_this();
    }

protected:
    std::string GetBody() override {
        std::string body_description = " car body with " + std::to_string(num_doors_) + " doors ";
        return body_description;
    }
};


class BikeBuilder : public VehicleBuilder<BikeBuilder> {
public:
    BikeBuilderPtr SetNumWheels(int num_wheels) final {
        if (num_wheels != 2) { std::cout << "Bikes have 2 wheels, silly!" << std::endl; }
        this->num_wheels = 2;
        return this->shared_from_this();
    }

protected:
    std::string GetBody() override {
        std::string body_description = " bike body ";
        return body_description;
    }
};

// Factories:
CarBuilderPtr CreateCarBuilder() { return std::make_shared<CarBuilder>(); }
BikeBuilderPtr CreateBikeBuilder() { return std::make_shared<BikeBuilder>(); }


int main() {
    // We can chain methods, even calling a base method and still accessing the concrete method.
    auto car = CreateCarBuilder()
            ->SetDisplacement(2000) // called on CRTP base
            ->SetNumDoors(2) // Non virtual mehod on derived class.
            ->Build(); // Build through base.

    std::cout << "\n  Assembly Line:  \n\n";

    // And we can collect different kinds of IBuilders and loop over them.
    auto bike = CreateBikeBuilder();

    std::vector<IVehicleBuilderPtr> list;
    list.push_back(car);
    list.push_back(bike);

    for (auto &b: list) {
        b
                ->SetDisplacement(1200)
                //This wires a polymorphic call all the way from IBuilder to the Concrete Car/Bike Builder.
                //Proving the bridge links the vtables!
                ->SetNumWheels(4)
                ->Build();
    }

    return 0;
}
