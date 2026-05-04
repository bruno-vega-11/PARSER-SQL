import random
import csv

nombres_hombres = [
    "Juan","Carlos","Pedro","Jose","Luis","Diego","Andres","Raul","Jorge","Ricardo",
    "Fernando","Alonso","Victor","Hugo","Martin","Oscar","Pablo","Enrique","Bruno","Guillermo",
    "Arturo","Cesar","Roberto","Esteban","Kevin","Renato","Sergio","Alfredo","Manuel","Ramon",
    "Tomas","Ernesto","Julio","Hector","Francisco","Daniel","Ruben","Gustavo","Alvaro","Samuel",
    "Leonardo","Marco","Nicolas","Edgar","Cristian","Brayan","Felipe","Joel","Javier","Axel"
]

nombres_mujeres = [
    "Ana","Lucia","Maria","Elena","Sofia","Camila","Valeria","Daniela","Paula","Andrea",
    "Patricia","Claudia","Rosa","Carla","Gloria","Teresa","Natalia","Sandra","Adriana","Silvia",
    "Beatriz","Monica","Jessica","Lorena","Melissa","Paola","Veronica","Diana","Angela","Fabiana",
    "Luisa","Mariana","Cecilia","Alicia","Laura","Noelia","Karen","Irene","Susana","Julia",
    "Rocio","Marta","Lidia","Carolina","Vanessa","Melany","Stefany","Bianca"
]

def generar_datos(cantidad=10000, archivo="data.csv"):
    with open(archivo, mode="w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        
        # Header
        writer.writerow(["dni", "nombre", "edad", "genero", "altura"])
        
        for i in range(cantidad):
            dni = 10000000 + i
            
            genero = random.choice(["M", "F"])
            
            if genero == "M":
                nombre = random.choice(nombres_hombres)
            else:
                nombre = random.choice(nombres_mujeres)
            
            edad = random.randint(18, 50)
            altura = round(random.uniform(1.55, 1.85), 2)
            
            writer.writerow([dni, nombre, edad, genero, altura])

# Ejecutar
generar_datos()