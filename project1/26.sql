SELECT name From CatchedPokemon,Pokemon Where pid = Pokemon.id and nickname LIKE '% %'
ORDER BY name DESC
