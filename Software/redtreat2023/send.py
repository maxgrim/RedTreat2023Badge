import paho.mqtt.publish as publish

uuid = "858e74f6-6ca1-4332-812d-13ed86cc2d90"
password = "<fill in password>"
hostname = "badge2023.treat.red"

publish.single(uuid, "/storage/xpboot.wav,0000FF", hostname=hostname, auth={"username": uuid, "password": password})
publish.single(uuid, "/storage/goat.wav,FF0000", hostname=hostname, auth={"username": uuid, "password": password})
publish.single(uuid, "/storage/airhorn.wav,FF00FF", hostname=hostname, auth={"username": uuid, "password": password})
publish.single(uuid, "/storage/rick.wav,00FF00", hostname=hostname, auth={"username": uuid, "password": password})