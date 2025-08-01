// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <functional>
#include <string>
#include <vector>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"
#include "parserUtils.h"

namespace odb {

lefTechLayerCutEnclosureRuleParser::lefTechLayerCutEnclosureRuleParser(
    lefinReader* l)
{
  lefin_ = l;
}

void lefTechLayerCutEnclosureRuleParser::parse(const std::string& s,
                                               odb::dbTechLayer* layer)
{
  processRules(s, [this, layer](const std::string& rule) {
    if (!parseSubRule(rule, layer)) {
      lefin_->warning(260,
                      "parse mismatch in layer property LEF58_ENCLOSURE for "
                      "layer {} :\"{}\"",
                      layer->getName(),
                      rule);
    }
  });
}
void lefTechLayerCutEnclosureRuleParser::setCutClass(
    std::string val,
    odb::dbTechLayerCutEnclosureRule* rule,
    odb::dbTechLayer* layer)
{
  auto cutClass = layer->findTechLayerCutClassRule(val.c_str());
  if (cutClass == nullptr) {
    lefin_->warning(
        601,
        "cut class {} not found for LEF58_ENCLOSURE rule for layer {}",
        val,
        layer->getName());
  } else {
    rule->setCutClass(cutClass);
    rule->setCutClassValid(true);
  }
}
void lefTechLayerCutEnclosureRuleParser::setInt(
    double val,
    odb::dbTechLayerCutEnclosureRule* rule,
    void (odb::dbTechLayerCutEnclosureRule::*func)(int))
{
  (rule->*func)(lefin_->dbdist(val));
}
bool lefTechLayerCutEnclosureRuleParser::parseSubRule(const std::string& s,
                                                      odb::dbTechLayer* layer)
{
  odb::dbTechLayerCutEnclosureRule* rule
      = odb::dbTechLayerCutEnclosureRule::create(layer);
  qi::rule<std::string::const_iterator, space_type> EOL
      = (lit("EOL")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setType,
             rule,
             odb::dbTechLayerCutEnclosureRule::ENC_TYPE::EOL)]
         >> double_[boost::bind(&lefTechLayerCutEnclosureRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerCutEnclosureRule::setEolWidth)]
         >> -(lit("MINLENGTH")[boost::bind(
                  &odb::dbTechLayerCutEnclosureRule::setEolMinLengthValid,
                  rule,
                  true)]
              >> double_[boost::bind(
                  &lefTechLayerCutEnclosureRuleParser::setInt,
                  this,
                  _1,
                  rule,
                  &odb::dbTechLayerCutEnclosureRule::setEolMinLength)])
         >> -lit("EOLONLY")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setEolOnly, rule, true)]
         >> -lit("SHORTEDGEONEOL")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setShortEdgeOnEol, rule, true)]
         >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setFirstOverhang)]
         >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setSecondOverhang)]
         >> -((lit("SIDESPACING")[boost::bind(
                   &odb::dbTechLayerCutEnclosureRule::setSideSpacingValid,
                   rule,
                   true)]
               >> double_[boost::bind(
                   &lefTechLayerCutEnclosureRuleParser::setInt,
                   this,
                   _1,
                   rule,
                   &odb::dbTechLayerCutEnclosureRule::setSpacing)]
               >> lit("EXTENSION") >> double_[boost::bind(
                   &lefTechLayerCutEnclosureRuleParser::setInt,
                   this,
                   _1,
                   rule,
                   &odb::dbTechLayerCutEnclosureRule::setForwardExtension)]
               >> double_[boost::bind(
                   &lefTechLayerCutEnclosureRuleParser::setInt,
                   this,
                   _1,
                   rule,
                   &odb::dbTechLayerCutEnclosureRule::setBackwardExtension)])
              | (lit("ENDSPACING")[boost::bind(
                     &odb::dbTechLayerCutEnclosureRule::setEndSpacingValid,
                     rule,
                     true)]
                 >> double_[boost::bind(
                     &lefTechLayerCutEnclosureRuleParser::setInt,
                     this,
                     _1,
                     rule,
                     &odb::dbTechLayerCutEnclosureRule::setSpacing)]
                 >> lit("EXTENSION") >> double_[boost::bind(
                     &lefTechLayerCutEnclosureRuleParser::setInt,
                     this,
                     _1,
                     rule,
                     &odb::dbTechLayerCutEnclosureRule::setExtension)])));

  qi::rule<std::string::const_iterator, space_type> DEFAULT
      = (double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setFirstOverhang)]
         >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setSecondOverhang)])
          [boost::bind(&odb::dbTechLayerCutEnclosureRule::setType,
                       rule,
                       odb::dbTechLayerCutEnclosureRule::ENC_TYPE::DEFAULT)];

  qi::rule<std::string::const_iterator, space_type> ENDSIDE
      = (-lit("OFFCENTERLINE")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setOffCenterLine, rule, true)]
         >> lit("END") >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setFirstOverhang)]
         >> lit("SIDE") >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setSecondOverhang)])
          [boost::bind(&odb::dbTechLayerCutEnclosureRule::setType,
                       rule,
                       odb::dbTechLayerCutEnclosureRule::ENC_TYPE::ENDSIDE)];

  qi::rule<std::string::const_iterator, space_type> HORZ_AND_VERT
      = (lit("HORIZONTAL") >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setFirstOverhang)]
         >> lit("VERTICAL") >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setSecondOverhang)])
          [boost::bind(
              &odb::dbTechLayerCutEnclosureRule::setType,
              rule,
              odb::dbTechLayerCutEnclosureRule::ENC_TYPE::HORZ_AND_VERT)];

  qi::rule<std::string::const_iterator, space_type> WIDTH_
      = (lit("WIDTH")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setWidthValid, rule, true)]
         >> double_[boost::bind(&lefTechLayerCutEnclosureRuleParser::setInt,
                                this,
                                _1,
                                rule,
                                &odb::dbTechLayerCutEnclosureRule::setMinWidth)]
         >> -lit("INCLUDEABUTTED")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setIncludeAbutted, rule, true)]
         >> -(lit("EXCEPTEXTRACUT")[boost::bind(
                  &odb::dbTechLayerCutEnclosureRule::setExceptExtraCut,
                  rule,
                  true)]
              >> double_[boost::bind(
                  &lefTechLayerCutEnclosureRuleParser::setInt,
                  this,
                  _1,
                  rule,
                  &odb::dbTechLayerCutEnclosureRule::setCutWithin)]
              >> -(lit("PRL")[boost::bind(
                       &odb::dbTechLayerCutEnclosureRule::setPrl, rule, true)]
                   | lit("NOSHAREDEDGE")[boost::bind(
                       &odb::dbTechLayerCutEnclosureRule::setNoSharedEdge,
                       rule,
                       true)])));

  qi::rule<std::string::const_iterator, space_type> LENGTH
      = (lit("LENGTH")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setLengthValid, rule, true)]
         >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setMinLength)]);

  qi::rule<std::string::const_iterator, space_type> EXTRACUT
      = (lit("EXTRACUT")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setExtraCutValid, rule, true)]
         >> -lit("EXTRAONLY")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setExtraOnly, rule, true)]);

  qi::rule<std::string::const_iterator, space_type> REDUNDANTCUT
      = (lit("REDUNDANTCUT")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setRedundantCutValid,
             rule,
             true)]
         >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setCutWithin)]);

  qi::rule<std::string::const_iterator, space_type> PARALLEL
      = (lit("PARALLEL")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setConcaveCornersValid,
             rule,
             true)]
         >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setParLength)]
         >> -double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setSecondParLength)]
         >> lit("WITHIN") >> double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setParWithin)]
         >> -double_[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setInt,
             this,
             _1,
             rule,
             &odb::dbTechLayerCutEnclosureRule::setSecondParWithin)]
         >> -(lit("BELOWENCLOSURE")[boost::bind(
                  &odb::dbTechLayerCutEnclosureRule::setBelowEnclosureValid,
                  rule,
                  true)]
              >> double_[boost::bind(
                  &lefTechLayerCutEnclosureRuleParser::setInt,
                  this,
                  _1,
                  rule,
                  &odb::dbTechLayerCutEnclosureRule::setBelowEnclosure)]));

  qi::rule<std::string::const_iterator, space_type> CONCAVECORNERS
      = (lit("CONCAVECORNERS")[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setConcaveCornersValid,
             rule,
             true)]
         >> int_[boost::bind(
             &odb::dbTechLayerCutEnclosureRule::setNumCorners, rule, _1)]);

  qi::rule<std::string::const_iterator, space_type> CUTCLASS
      = (-(lit("CUTCLASS") >> _string)[boost::bind(
             &lefTechLayerCutEnclosureRuleParser::setCutClass,
             this,
             _1,
             rule,
             layer)]
         >> -(lit("ABOVE")[boost::bind(
                  &odb::dbTechLayerCutEnclosureRule::setAbove, rule, true)]
              | lit("BELOW")[boost::bind(
                  &odb::dbTechLayerCutEnclosureRule::setBelow, rule, true)]));
  qi::rule<std::string::const_iterator, space_type> ENCLOSURE
      = (lit("ENCLOSURE") >> CUTCLASS
         >> (EOL | DEFAULT | ENDSIDE | HORZ_AND_VERT) >> -WIDTH_ >> -LENGTH
         >> -EXTRACUT >> -REDUNDANTCUT >> -PARALLEL >> -CONCAVECORNERS
         >> lit(";"));
  auto first = s.begin();
  auto last = s.end();
  bool valid = qi::phrase_parse(first, last, ENCLOSURE, space) && first == last;
  if (!valid) {
    odb::dbTechLayerCutEnclosureRule::destroy(rule);
  }
  return valid;
}

}  // namespace odb
