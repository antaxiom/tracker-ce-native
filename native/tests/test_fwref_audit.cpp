#include <catch2/catch_test_macros.hpp>

#include "audit.h"

TEST_CASE("fwref audit separates unsupported calls from approximations", "[fwref][audit]") {
    fwref::resetAudit();
    fwref::setStrict(false);

    fwref::auditCall("TEST_UNSUPPORTED", fwref::AuditClass::PixelAffecting);
    fwref::auditCall("TEST_APPROX", fwref::AuditClass::Approximation);
    fwref::auditCall("TEST_SAFE", fwref::AuditClass::IgnoredSafe);

    CHECK(fwref::auditFailures() == 1);
    CHECK(fwref::auditWarnings() == 1);

    fwref::resetAudit();
    CHECK(fwref::auditFailures() == 0);
    CHECK(fwref::auditWarnings() == 0);
}

TEST_CASE("fwref strict mode is opt-in", "[fwref][audit]") {
    fwref::setStrict(true);
    CHECK(fwref::strict());
    fwref::setStrict(false);
    CHECK_FALSE(fwref::strict());
}
