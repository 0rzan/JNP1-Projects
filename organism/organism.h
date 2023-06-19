#ifndef ORGANISM_H
#define ORGANISM_H

#include <concepts>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>

template <std::equality_comparable species_t, bool can_eat_meat,
          bool can_eat_plants>
class Organism {

public:
  uint64_t vitality;
  species_t const species;

  constexpr Organism(species_t const &species, uint64_t vitality)
      : vitality(vitality), species(species) {}

  constexpr uint64_t get_vitality() const { return vitality; };

  constexpr species_t get_species() const { return species; }

  constexpr bool is_dead() const { return vitality == 0; }
};

template <typename species_t>
using Carnivore = Organism<species_t, true, false>;
template <typename species_t> using Omnivore = Organism<species_t, true, true>;
template <typename species_t>
using Herbivore = Organism<species_t, false, true>;
template <typename species_t> using Plant = Organism<species_t, false, false>;

template <typename species_t, bool sp1_eats_m, bool sp1_eats_p, bool sp2_eats_m,
          bool sp2_eats_p>
  requires(sp1_eats_m || sp1_eats_p || sp2_eats_m || sp2_eats_p)
constexpr std::tuple<Organism<species_t, sp1_eats_m, sp1_eats_p>,
                     Organism<species_t, sp2_eats_m, sp2_eats_p>,
                     std::optional<Organism<species_t, sp1_eats_m, sp1_eats_p>>>
encounter(Organism<species_t, sp1_eats_m, sp1_eats_p> organism1,
          Organism<species_t, sp2_eats_m, sp2_eats_p> organism2) {
  if (organism1.is_dead() ||
      organism2.is_dead()) { // jezeli ktorys nie zyje (3.)
    return {organism1, organism2, {}};
  }
  if constexpr (sp1_eats_m == sp2_eats_m && sp1_eats_p == sp2_eats_p) {
    // jezeli sa zgodne preferencje
    // zywieniowe (1.)
    if (organism1.get_species() == organism2.get_species())
      // spotkanie dwoch zwierzat tego samego gatunku (4.)
      return {organism1,
              organism2,
              {organism1.get_species(),
               (organism1.get_vitality() + organism2.get_vitality()) / 2}};
    else {
      if (sp1_eats_m) { // jezeli obaj sa miesozerni lub wszystkozerni (6.)
        if (organism1.get_vitality() > organism2.get_vitality())
          return {{organism1.get_species(),
                   (organism1.get_vitality() + organism2.get_vitality() / 2)},
                  {organism2.get_species(), 0},
                  {}};
        else if (organism1.get_vitality() == organism2.get_vitality())
          return {
              {organism1.get_species(), 0}, {organism2.get_species(), 0}, {}};
        else
          return {{organism1.get_species(), 0},
                  {organism2.get_species(),
                   (organism2.get_vitality() + organism1.get_vitality() / 2)},
                  {}};
      } else // jezeli obaj sa roslinozerni (5.)
        return {organism1, organism2, {}};
    }
  } else if ((!sp1_eats_m && !sp1_eats_p) ||
             (!sp2_eats_m &&
              !sp2_eats_p)) { // jezeli jeden z nich jest roslina (7.)
    // jesli drugi jest roslina
    if (sp1_eats_p) // jesli pierwszy jest roslinozerca lub wszystkozerca
      return {{organism1.get_species(),
               (organism1.get_vitality() + organism2.get_vitality())},
              {organism2.get_species(), 0},
              {}};
    else // jesli pierwszy jest miesozerca
      return {organism1, organism2, {}};
    // jesli pierwszy jest roslina
    if (sp2_eats_p) // jesli drugi jest roslinozerca lub wszystkozerca
      return {{organism1.get_species(), 0},
              {organism2.get_species(),
               (organism1.get_vitality() + organism2.get_vitality())},
              {}};
    else // jesli drugi jest miesozerca
      return {organism1, organism2, {}};
  } else { // jesli zaden z nich nie jest roslina i nie maja zgodnych
           // preferencji zywieniowych
    if (sp1_eats_m && sp2_eats_m) { // spotyknie miesozercy i wszystkozercy (6.)
      if (organism1.get_vitality() > organism2.get_vitality())
        return {{organism1.get_species(),
                 (organism1.get_vitality() + organism2.get_vitality() / 2)},
                {organism2.get_species(), 0},
                {}};
      else if (organism1.get_vitality() == organism2.get_vitality())
        return {{organism1.get_species(), 0}, {organism2.get_species(), 0}, {}};
      else
        return {{organism1.get_species(), 0},
                {organism2.get_species(),
                 (organism2.get_vitality() + organism1.get_vitality() / 2)},
                {}};
    } else { // jezeli zdolnosc konsumpcji zachodzi w jedna strone (8.)
      if (organism1.get_vitality() > organism2.get_vitality()) {
        if (sp1_eats_p) // jezeli drugi jest roslinozerca
          return {organism1, organism2, {}};
        else
          return {{organism1.get_species(),
                   (organism1.get_vitality() + organism2.get_vitality() / 2)},
                  {organism2.get_species(), 0},
                  {}};
      } else if (organism1.get_vitality() == organism2.get_vitality())
        return {organism1, organism2, {}};
      else {
        if (sp2_eats_p) // jezeli pierwszy jest roslinozerca
          return {organism1, organism2, {}};
        else
          return {{organism1.get_species(), 0},
                  {organism2.get_species(),
                   (organism2.get_vitality() + organism1.get_vitality() / 2)},
                  {}};
      }
    }
  }
}

template <typename species_t, bool sp1_eats_m, bool sp1_eats_p, typename Arg1,
          typename... Args>
constexpr Organism<species_t, sp1_eats_m, sp1_eats_p>
encounter_series(Organism<species_t, sp1_eats_m, sp1_eats_p> organism1,
                 Arg1 arg1, Args... args) {
  return encounter_series(encounter_series(organism1, arg1), args...);
}

template <typename species_t, bool sp1_eats_m, bool sp1_eats_p, typename Arg>
constexpr Organism<species_t, sp1_eats_m, sp1_eats_p>
encounter_series(Organism<species_t, sp1_eats_m, sp1_eats_p> organism1,
                 Arg arg) {
  return std::get<0>(encounter(organism1, arg));
}

#endif