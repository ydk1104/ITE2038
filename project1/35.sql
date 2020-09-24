SELECT name From Pokemon, (SELECT before_id From Evolution Where before_id > after_id) As T Where id = before_id
ORDER BY name;
