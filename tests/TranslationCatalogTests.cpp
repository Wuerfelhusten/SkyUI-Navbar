#include "TranslationCatalog.h"
#include <stdexcept>

int main()
{
	const auto english = Navbar::ParseTranslationCatalog("$Inventory\tInventory\n$Magic\tMagic\n$CustomSkills\tCustom Skills\n$Case\tMAGIC\n");
	const auto german = Navbar::ParseTranslationCatalog("$Magic\tMagie\n");
	if (Navbar::ResolveTranslation("$Magic", english, {}) != "Magic" ||
		Navbar::ResolveTranslation("$Inventory", german, english) != "Inventory" ||
		Navbar::ResolveTranslation("$Magic", german, english) != "Magie" ||
		Navbar::ResolveTranslation("$CustomSkills", german, english) != "Custom Skills" ||
		Navbar::ResolveTranslation("$Case", {}, english) != "MAGIC" ||
		Navbar::ResolveTranslation("$Missing", german, english) != "$Missing") {
		throw std::runtime_error("Exact spelling, language priority or per-key English fallback changed");
	}
	const auto catalog = Navbar::ParseTranslationCatalog(
		"$One\tHello\r\n$Two\t{keyboard} / {gamepad}\n\n$Three\tGr\xC3\xB6\xC3\x9F"
		"e");
	if (catalog.size() != 3 || catalog.at("$One") != "Hello" || catalog.at("$Two") != "{keyboard} / {gamepad}" ||
		catalog.at("$Three") !=
			"Gr\xC3\xB6\xC3\x9F"
			"e") {
		throw std::runtime_error("Catalog did not preserve translated text/placeholders/UTF-8");
	}
	for (const auto invalid : { "$Key\tOne\n$Key\tTwo", "$Key\t", "$Key", "$Key\tOne\tTwo" }) {
		bool rejected = false;
		try {
			Navbar::ParseTranslationCatalog(invalid);
		} catch (const std::runtime_error&) {
			rejected = true;
		}
		if (!rejected) {
			throw std::runtime_error("Malformed catalog accepted");
		}
	}
}
